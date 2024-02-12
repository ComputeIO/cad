/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eeschema_jobs_handler.h"
#include <pgm_base.h>
#include <cli/exit_codes.h>
#include <sch_plotter.h>
#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_export_sch_plot.h>
#include <jobs/job_sym_export_svg.h>
#include <jobs/job_sym_upgrade.h>
#include <schematic.h>
#include <wx/crt.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <memory>
#include <connection_graph.h>
#include "eeschema_helpers.h"
#include <sch_painter.h>
#include <locale_io.h>
#include <erc.h>
#include <wildcards_and_files_ext.h>
#include <plotters/plotters_pslike.h>
#include <drawing_sheet/ds_data_model.h>

#include <settings/settings_manager.h>

#include <sch_file_versions.h>
#include <sch_plugins/kicad/sch_sexpr_lib_plugin_cache.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_spice.h>
#include <netlist_exporter_spice_model.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>


EESCHEMA_JOBS_HANDLER::EESCHEMA_JOBS_HANDLER()
{
    Register( "pythonbom",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPythonBom, this, std::placeholders::_1 ) );
    Register( "netlist",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportNetlist, this, std::placeholders::_1 ) );
    Register( "plot",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobExportPlot, this, std::placeholders::_1 ) );
    Register( "symupgrade",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymUpgrade, this, std::placeholders::_1 ) );
    Register( "symsvg",
              std::bind( &EESCHEMA_JOBS_HANDLER::JobSymExportSvg, this, std::placeholders::_1 ) );
}


void EESCHEMA_JOBS_HANDLER::InitRenderSettings( KIGFX::SCH_RENDER_SETTINGS* aRenderSettings,
                                                const wxString& aTheme, SCHEMATIC* aSch )
{
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( aTheme );
    aRenderSettings->LoadColors( cs );

    aRenderSettings->SetDefaultPenWidth( aSch->Settings().m_DefaultLineWidth );
    aRenderSettings->m_LabelSizeRatio = aSch->Settings().m_LabelSizeRatio;
    aRenderSettings->m_TextOffsetRatio = aSch->Settings().m_TextOffsetRatio;
    aRenderSettings->m_PinSymbolSize = aSch->Settings().m_PinSymbolSize;

    aRenderSettings->SetDashLengthRatio( aSch->Settings().m_DashedLineDashRatio );
    aRenderSettings->SetGapLengthRatio( aSch->Settings().m_DashedLineGapRatio );

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.
    wxString filename = DS_DATA_MODEL::ResolvePath( aSch->Settings().m_SchDrawingSheetFileName,
                                                    aSch->Prj().GetProjectPath() );

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename ) )
        wxFprintf( stderr, _( "Error loading drawing sheet." ) + wxS( "\n" ) );
}


REPORTER& EESCHEMA_JOBS_HANDLER::Report( const wxString& aText, SEVERITY aSeverity )
{
    if( aSeverity == RPT_SEVERITY_ERROR )
        wxFprintf( stderr, wxS( "%s\n" ), aText );
    else
        wxPrintf( wxS( "%s\n" ), aText );

    return *this;
}


int EESCHEMA_JOBS_HANDLER::JobExportPlot( JOB* aJob )
{
    JOB_EXPORT_SCH_PLOT* aPlotJob = dynamic_cast<JOB_EXPORT_SCH_PLOT*>( aJob );

    if( !aPlotJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aPlotJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    std::unique_ptr<KIGFX::SCH_RENDER_SETTINGS> renderSettings =
            std::make_unique<KIGFX::SCH_RENDER_SETTINGS>();
    InitRenderSettings( renderSettings.get(), aPlotJob->settings.m_theme, sch );

    std::unique_ptr<SCH_PLOTTER> schPlotter = std::make_unique<SCH_PLOTTER>( sch );
    schPlotter->Plot( aPlotJob->m_plotFormat, aPlotJob->settings, renderSettings.get(), this );

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportNetlist( JOB* aJob )
{
    JOB_EXPORT_SCH_NETLIST* aNetJob = dynamic_cast<JOB_EXPORT_SCH_NETLIST*>( aJob );

    if( !aNetJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );

    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                        // We're only interested in the end result -- either errors or not
                    } )
            > 0 )
        {
            wxPrintf( _( "Warning: schematic has annotation errors, please use the schematic editor to fix them\n" ) );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
    {
        wxPrintf( _( "Warning: duplicate sheet names.\n" ) );
    }


    std::unique_ptr<NETLIST_EXPORTER_BASE> helper;
    unsigned netlistOption = 0;

    wxString fileExt;

    switch( aNetJob->format )
    {
    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR:
        fileExt = NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_KICAD>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2:
        fileExt = OrCadPcb2NetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_ORCADPCB2>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR:
        fileExt = CadstarNetlistFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_CADSTAR>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE:
        fileExt = SpiceFileExtension;
        netlistOption = NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL:
        fileExt = SpiceFileExtension;
        helper = std::make_unique<NETLIST_EXPORTER_SPICE_MODEL>( sch );
        break;

    case JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML:
        fileExt = wxS( "xml" );
        helper = std::make_unique<NETLIST_EXPORTER_XML>( sch );
        break;
    default:
        wxFprintf( stderr, _( "Unknown netlist format.\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }


    if( aNetJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() );
        fn.SetExt( fileExt );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = helper->WriteNetlist( aNetJob->m_outputFile, netlistOption, *this );

    if(!res)
    {
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobExportPythonBom( JOB* aJob )
{
    JOB_EXPORT_SCH_PYTHONBOM* aNetJob = dynamic_cast<JOB_EXPORT_SCH_PYTHONBOM*>( aJob );

    SCHEMATIC* sch = EESCHEMA_HELPERS::LoadSchematic( aNetJob->m_filename, SCH_IO_MGR::SCH_KICAD );

    if( sch == nullptr )
    {
        wxFprintf( stderr, _( "Failed to load schematic file\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Annotation warning check
    SCH_REFERENCE_LIST referenceList;
    sch->GetSheets().GetSymbols( referenceList );

    if( referenceList.GetCount() > 0 )
    {
        if( referenceList.CheckAnnotation(
                    []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                    {
                        // We're only interested in the end result -- either errors or not
                    } )
            > 0 )
        {
            wxPrintf( _( "Warning: schematic has annotation errors, please use the schematic "
                         "editor to fix them\n" ) );
        }
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( sch );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
        wxPrintf( _( "Warning: duplicate sheet names.\n" ) );

    std::unique_ptr<NETLIST_EXPORTER_XML> xmlNetlist =
            std::make_unique<NETLIST_EXPORTER_XML>( sch );

    if( aNetJob->m_outputFile.IsEmpty() )
    {
        wxFileName fn = sch->GetFileName();
        fn.SetName( fn.GetName() + "-bom" );
        fn.SetExt( XmlFileExtension );

        aNetJob->m_outputFile = fn.GetFullName();
    }

    bool res = xmlNetlist->WriteNetlist( aNetJob->m_outputFile, GNL_OPT_BOM, *this );

    if( !res )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::doSymExportSvg( JOB_SYM_EXPORT_SVG*         aSvgJob,
                                           KIGFX::SCH_RENDER_SETTINGS* aRenderSettings,
                                           LIB_SYMBOL*                 symbol )
{
    wxASSERT( symbol != nullptr );

    if( symbol == nullptr )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    LIB_SYMBOL* symbolToPlot = symbol;

    // if the symbol is an alias, then the draw items are stored in the parent
    if( symbol->IsAlias() )
    {
        LIB_SYMBOL_SPTR parent = symbol->GetParent().lock();
        symbolToPlot = parent.get();
    }

    if( aSvgJob->m_includeHiddenPins )
    {
        // horrible hack, TODO overhaul the Plot method to handle this
        for( LIB_ITEM& item : symbolToPlot->GetDrawItems() )
        {
            if( item.Type() != LIB_PIN_T )
                continue;

            LIB_PIN& pin = static_cast<LIB_PIN&>( item );
            pin.SetVisible( true );
        }
    }

    // iterate from unit 1, unit 0 would be "all units" which we don't want
    for( int unit = 1; unit < symbol->GetUnitCount() + 1; unit++ )
    {
        for( int convert = 1; convert < ( symbol->HasConversion() ? 2 : 1 ) + 1; ++convert )
        {
            wxString   filename;
            wxFileName fn;

            fn.SetPath( aSvgJob->m_outputDirectory );
            fn.SetExt( SVGFileExtension );

            //simplify the name if its single unit
            if( symbol->IsMulti() )
            {
                filename = wxString::Format( "%s_%d", symbol->GetName().Lower(), unit );

                if( convert == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                wxPrintf( _( "Plotting symbol '%s' unit %d to '%s'\n" ), symbol->GetName(), unit,
                          fn.GetFullPath() );
            }
            else
            {
                filename = symbol->GetName().Lower();

                if( convert == 2 )
                    filename += wxS( "_demorgan" );

                fn.SetName( filename );
                wxPrintf( _( "Plotting symbol '%s' to '%s'\n" ), symbol->GetName(), fn.GetFullPath() );
            }

            // Get the symbol bounding box to fit the plot page to it
            BOX2I     symbolBB = symbol->Flatten()->GetUnitBoundingBox( unit, convert, false );
            PAGE_INFO pageInfo( PAGE_INFO::Custom );
            pageInfo.SetHeightMils( schIUScale.IUToMils( symbolBB.GetHeight() * 1.2 ) );
            pageInfo.SetWidthMils( schIUScale.IUToMils( symbolBB.GetWidth() * 1.2 ) );

            SVG_PLOTTER* plotter = new SVG_PLOTTER();
            plotter->SetRenderSettings( aRenderSettings );
            plotter->SetPageSettings( pageInfo );
            plotter->SetColorMode( !aSvgJob->m_blackAndWhite );

        wxPoint      plot_offset;
        const double scale = 1.0;

            // Currently, plot units are in decimil
            plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, scale, false );

            plotter->SetCreator( wxT( "Eeschema-SVG" ) );

            if( !plotter->OpenFile( fn.GetFullPath() ) )
            {
                wxFprintf( stderr, _( "Unable to open destination '%s'" ) + wxS( "\n" ), fn.GetFullPath() );

                delete plotter;
                return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
            }

            LOCALE_IO toggle;

            plotter->StartPlot( wxT( "1" ) );

            bool      background = true;
            TRANSFORM temp; // Uses default transform
            VECTOR2I  plotPos;

            plotPos.x = pageInfo.GetWidthIU( schIUScale.IU_PER_MILS ) / 2;
            plotPos.y = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS ) / 2;

            // note, we want the fields from the original symbol pointer (in case of non-alias)
            symbolToPlot->Plot( plotter, unit, convert, background, plotPos, temp, false );
            symbol->PlotLibFields( plotter, unit, convert, background, plotPos, temp, false,
                                   aSvgJob->m_includeHiddenFields );

            symbolToPlot->Plot( plotter, unit, convert, !background, plotPos, temp, false );
            symbol->PlotLibFields( plotter, unit, convert, !background, plotPos, temp, false,
                                   aSvgJob->m_includeHiddenFields );

            plotter->EndPlot();
            delete plotter;
        }
    }

    return CLI::EXIT_CODES::OK;
}


int EESCHEMA_JOBS_HANDLER::JobSymExportSvg( JOB* aJob )
{
    JOB_SYM_EXPORT_SVG* svgJob = dynamic_cast<JOB_SYM_EXPORT_SVG*>( aJob );

    wxFileName fn( svgJob->m_libraryPath );
    fn.MakeAbsolute();

    SCH_SEXPR_PLUGIN_CACHE schLibrary( fn.GetFullPath() );

    try
    {
        schLibrary.Load();
    }
    catch( ... )
    {
        wxFprintf( stderr, _( "Unable to load library\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    LIB_SYMBOL* symbol = nullptr;

    if( !svgJob->m_symbol.IsEmpty() )
    {
        // See if the selected symbol exists
        symbol = schLibrary.GetSymbol( svgJob->m_symbol );

        if( !symbol )
        {
            wxFprintf( stderr, _( "There is no symbol selected to save." ) + wxS( "\n" ) );
            return CLI::EXIT_CODES::ERR_ARGS;
        }
    }

    if( !svgJob->m_outputDirectory.IsEmpty() && !wxDir::Exists( svgJob->m_outputDirectory ) )
    {
        wxFileName::Mkdir( svgJob->m_outputDirectory );
    }

    KIGFX::SCH_RENDER_SETTINGS renderSettings;
    COLOR_SETTINGS* cs = Pgm().GetSettingsManager().GetColorSettings( svgJob->m_colorTheme );
    renderSettings.LoadColors( cs );
    renderSettings.SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );

    int exitCode = CLI::EXIT_CODES::OK;

    if( symbol )
    {
        exitCode = doSymExportSvg( svgJob, &renderSettings, symbol );
    }
    else
    {
        // Just plot all the symbols we can
        const LIB_SYMBOL_MAP& libSymMap = schLibrary.GetSymbolMap();

        for( const std::pair<const wxString, LIB_SYMBOL*>& entry : libSymMap )
        {
            exitCode = doSymExportSvg( svgJob, &renderSettings, entry.second );

            if( exitCode != CLI::EXIT_CODES::OK )
                break;
        }
    }

    return exitCode;
}


int EESCHEMA_JOBS_HANDLER::JobSymUpgrade( JOB* aJob )
{
    JOB_SYM_UPGRADE* upgradeJob = dynamic_cast<JOB_SYM_UPGRADE*>( aJob );

    wxFileName fn( upgradeJob->m_libraryPath );
    fn.MakeAbsolute();

    SCH_SEXPR_PLUGIN_CACHE schLibrary( fn.GetFullPath() );

    try
    {
        schLibrary.Load();
    }
    catch( ... )
    {
        wxFprintf( stderr, _( "Unable to load library\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
    {
        if( wxFile::Exists( upgradeJob->m_outputLibraryPath ) )
        {
            wxFprintf( stderr, _( "Output path must not conflict with existing path\n" ) );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    bool shouldSave = upgradeJob->m_force
                      || schLibrary.GetFileFormatVersionAtLoad() < SEXPR_SYMBOL_LIB_FILE_VERSION;

    if( shouldSave )
    {
        wxPrintf( _( "Saving symbol library in updated format\n" ) );

        try
        {
            if( !upgradeJob->m_outputLibraryPath.IsEmpty() )
            {
                schLibrary.SetFileName( upgradeJob->m_outputLibraryPath );
            }

            schLibrary.SetModified();
            schLibrary.Save();
        }
        catch( ... )
        {
            wxFprintf( stderr, _( "Unable to save library\n" ) );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
    }
    else
    {
        wxPrintf( _( "Symbol library was not updated\n" ) );
    }

    return CLI::EXIT_CODES::OK;
}
