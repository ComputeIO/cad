/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <config.h>     // Needed for MSW compilation
#include <common.h>
#include <locale_io.h>
#include <fmt/core.h>
#include <paths.h>

#include "ngspice_circuit_model.h"
#include "ngspice.h"
#include "spice_reporter.h"
#include "spice_settings.h"

#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/log.h>

#include <stdexcept>
#include <algorithm>


/**
 * Flag to enable debug output of Ngspice simulator.
 *
 * Use "KICAD_NGSPICE" to enable Ngspice simulator tracing.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceNgspice = wxT( "KICAD_NGSPICE" );


NGSPICE::NGSPICE() : m_error( false )
{
    init_dll();
}


void NGSPICE::Init( const SPICE_SIMULATOR_SETTINGS* aSettings )
{
    Command( "reset" );

    for( const std::string& command : GetSettingCommands() )
    {
        wxLogTrace( traceNgspice, "Sending Ngspice configuration command '%s'.", command );
        Command( command );
    }
}


std::vector<std::string> NGSPICE::AllPlots() const
{
    LOCALE_IO c_locale; // ngspice works correctly only with C locale
    char*     currentPlot = ngSpice_CurPlot();
    char**    allPlots    = ngSpice_AllVecs( currentPlot );
    int       noOfPlots   = 0;

    std::vector<std::string> retVal;

    if( allPlots != nullptr )
    {
        for( char** plot = allPlots; *plot != nullptr; plot++ )
            noOfPlots++;

        retVal.reserve( noOfPlots );

        for( int i = 0; i < noOfPlots; i++, allPlots++ )
        {
            std::string vec = *allPlots;
            retVal.push_back( vec );
        }
    }


    return retVal;
}


std::vector<COMPLEX> NGSPICE::GetPlot( const std::string& aName, int aMaxLen )
{
    LOCALE_IO            c_locale;       // ngspice works correctly only with C locale
    std::vector<COMPLEX> data;
    vector_info*         vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_realdata[i], 0.0 );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetRealPlot( const std::string& aName, int aMaxLen )
{
    LOCALE_IO           c_locale;       // ngspice works correctly only with C locale
    std::vector<double> data;
    vector_info*        vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
            {
                wxASSERT( vi->v_compdata[i].cx_imag == 0.0 );
                data.push_back( vi->v_compdata[i].cx_real );
            }
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetImagPlot( const std::string& aName, int aMaxLen )
{
    LOCALE_IO           c_locale;       // ngspice works correctly only with C locale
    std::vector<double> data;
    vector_info*        vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_compdata[i].cx_imag );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetMagPlot( const std::string& aName, int aMaxLen )
{
    LOCALE_IO           c_locale;       // ngspice works correctly only with C locale
    std::vector<double> data;
    vector_info*        vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( hypot( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag ) );
        }
    }

    return data;
}


std::vector<double> NGSPICE::GetPhasePlot( const std::string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    std::vector<double> data;
    vector_info*        vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( 0.0 );      // well, that's life
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( atan2( vi->v_compdata[i].cx_imag, vi->v_compdata[i].cx_real ) );
        }
    }

    return data;
}


bool NGSPICE::Attach( const std::shared_ptr<SIMULATION_MODEL>& aModel, REPORTER& aReporter )
{
    NGSPICE_CIRCUIT_MODEL* model = dynamic_cast<NGSPICE_CIRCUIT_MODEL*>( aModel.get() );
    STRING_FORMATTER formatter;

    if( model && model->GetNetlist( &formatter, aReporter ) )
    {
        SIMULATOR::Attach( aModel, aReporter );

        LoadNetlist( formatter.GetString() );

        return true;
    }
    else
    {
        SIMULATOR::Attach( nullptr, aReporter );

        return false;
    }
}


bool NGSPICE::LoadNetlist( const std::string& aNetlist )
{
    LOCALE_IO          c_locale;       // ngspice works correctly only with C locale
    std::vector<char*> lines;
    std::stringstream  ss( aNetlist );

    m_netlist = "";

    while( !ss.eof() )
    {
        char line[1024];
        ss.getline( line, sizeof( line ) );
        lines.push_back( strdup( line ) );
        m_netlist += std::string( line ) + std::string( "\n" );
    }

    lines.push_back( nullptr ); // sentinel, as requested in ngSpice_Circ description

    Command( "remcirc" );
    bool success = !ngSpice_Circ( lines.data() );

    for( char* line : lines )
        free( line );

    return success;
}


bool NGSPICE::Run()
{
    LOCALE_IO toggle;                       // ngspice works correctly only with C locale
    bool success = Command( "bg_run" );     // bg_* commands execute in a separate thread

    if( success )
    {
        // wait for end of simulation.
        // calling wxYield() allows printing activity, and stopping ngspice from GUI
        // Also note: do not user wxSafeYield, because when using it we cannot stop
        // ngspice from the GUI
        do
        {
            wxMilliSleep( 50 );
            wxYield();
        } while( ngSpice_running() );
    }

    return success;
}


bool NGSPICE::Stop()
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return Command( "bg_halt" );    // bg_* commands execute in a separate thread
}


bool NGSPICE::IsRunning()
{
    // No need to use C locale here
    return ngSpice_running();
}


bool NGSPICE::Command( const std::string& aCmd )
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    validate();
    return !ngSpice_Command( (char*) aCmd.c_str() );
}


wxString NGSPICE::GetXAxis( SIM_TYPE aType ) const
{
    switch( aType )
    {
    case ST_AC:
    case ST_NOISE:
        return wxS( "frequency" );

    case ST_DC:
        // find plot, which ends with "-sweep"
        for( wxString plot : AllPlots() )
        {
            if( plot.Lower().EndsWith( wxS( "-sweep" ) ) )
                return plot;
        }

        return wxS( "sweep" );

    case ST_TRANSIENT:
        return wxS( "time" );

    default:
        return wxEmptyString;
    }
}


std::vector<std::string> NGSPICE::GetSettingCommands() const
{
    const NGSPICE_SIMULATOR_SETTINGS* settings =
            dynamic_cast<const NGSPICE_SIMULATOR_SETTINGS*>( Settings().get() );

    std::vector<std::string> commands;

    wxCHECK( settings, commands );

    switch( settings->GetModelMode() )
    {
    case NGSPICE_MODEL_MODE::USER_CONFIG:
        break;

    case NGSPICE_MODEL_MODE::NGSPICE:
        commands.emplace_back( "unset ngbehavior" );
        break;

    case NGSPICE_MODEL_MODE::PSPICE:
        commands.emplace_back( "set ngbehavior=psa" );
        break;

    case NGSPICE_MODEL_MODE::LTSPICE:
        commands.emplace_back( "set ngbehavior=lta" );
        break;

    case NGSPICE_MODEL_MODE::LT_PSPICE:
        commands.emplace_back( "set ngbehavior=ltpsa" );
        break;

    case NGSPICE_MODEL_MODE::HSPICE:
        commands.emplace_back( "set ngbehavior=hsa" );
        break;

    default:
        wxFAIL_MSG( wxString::Format( "Undefined NGSPICE_MODEL_MODE %d.",
                                      settings->GetModelMode() ) );
        break;
    }

    return commands;
}


const std::string NGSPICE::GetNetlist() const
{
    return m_netlist;
}


void NGSPICE::init_dll()
{
    if( m_initialized )
        return;

    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    const wxStandardPaths& stdPaths = wxStandardPaths::Get();

    ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, NULL, NULL, &cbBGThreadRunning,
                  this );

    // Load a custom spinit file, to fix the problem with loading .cm files
    // Switch to the executable directory, so the relative paths are correct
    wxString cwd( wxGetCwd() );
    wxFileName exeDir( stdPaths.GetExecutablePath() );
    wxSetWorkingDirectory( exeDir.GetPath() );

    // Find *.cm files
    std::string cmPath = findCmPath();

    // __CMPATH is used in custom spinit file to point to the codemodels directory
    if( !cmPath.empty() )
        Command( "set __CMPATH=\"" + cmPath + "\"" );

    // Possible relative locations for spinit file
    const std::vector<std::string> spiceinitPaths =
    {
        ".",
#ifdef __WXMAC__
        stdPaths.GetPluginsDir().ToStdString() + "/sim/ngspice/scripts",
        wxFileName( stdPaths.GetExecutablePath() ).GetPath().ToStdString() +
                "/../../../../../Contents/PlugIns/sim/ngspice/scripts"
#endif
        "../share/kicad",
        "../share",
        "../../share/kicad",
        "../../share"
    };

    bool foundSpiceinit = false;

    for( const auto& path : spiceinitPaths )
    {
        wxLogTrace( traceNgspice, "ngspice init script search path: %s", path );

        if( loadSpinit( path + "/spiceinit" ) )
        {
            wxLogTrace( traceNgspice, "ngspice path found in: %s", path );
            foundSpiceinit = true;
            break;
        }
    }

    // Last chance to load codemodel files, we have not found
    // spiceinit file, but we know the path to *.cm files
    if( !foundSpiceinit && !cmPath.empty() )
        loadCodemodels( cmPath );

    // Restore the working directory
    wxSetWorkingDirectory( cwd );

    // Workarounds to avoid hang ups on certain errors
    // These commands have to be called, no matter what is in the spinit file
    // We have to allow interactive for user-defined signals.  Hopefully whatever bug this was
    // meant to address has gone away in the last 5 years...
    //Command( "unset interactive" );
    Command( "set noaskquit" );
    Command( "set nomoremode" );

    // reset and remcirc give an error if no circuit is loaded, so load an empty circuit at the
    // start.

    std::vector<char*> lines;
    lines.push_back( strdup( "*" ) );
    lines.push_back( strdup( ".end" ) );
    lines.push_back( nullptr ); // Sentinel.

    ngSpice_Circ( lines.data() );

    for( auto line : lines )
        free( line );

    m_initialized = true;
}


bool NGSPICE::loadSpinit( const std::string& aFileName )
{
    if( !wxFileName::FileExists( aFileName ) )
        return false;

    wxTextFile file;

    if( !file.Open( aFileName ) )
        return false;

    for( wxString& cmd = file.GetFirstLine(); !file.Eof(); cmd = file.GetNextLine() )
        Command( cmd.ToStdString() );

    return true;
}


std::string NGSPICE::findCmPath() const
{
    const std::vector<std::string> cmPaths =
    {
#ifdef __WXMAC__
        "/Applications/ngspice/lib/ngspice",
        "Contents/Frameworks",
        wxStandardPaths::Get().GetPluginsDir().ToStdString() + "/sim/ngspice",
        wxFileName( wxStandardPaths::Get().GetExecutablePath() ).GetPath().ToStdString() +
                "/../../../../../Contents/PlugIns/sim/ngspice",
        "../Plugins/sim/ngspice",
#endif
        "../lib/ngspice",
        "../../lib/ngspice",
        "lib/ngspice",
        "ngspice"
    };

    for( const auto& path : cmPaths )
    {
        wxLogTrace( traceNgspice, "ngspice code models search path: %s", path );

        if( wxFileName::FileExists( path + "/spice2poly.cm" ) )
        {
            wxLogTrace( traceNgspice, "ngspice code models found in: %s", path );
            return path;
        }
    }

    return std::string();
}


bool NGSPICE::loadCodemodels( const std::string& aPath )
{
    wxArrayString cmFiles;
    size_t count = wxDir::GetAllFiles( aPath, &cmFiles );

    for( const auto& cm : cmFiles )
        Command( fmt::format( "codemodel '{}'", cm.ToStdString() ) );

    return count != 0;
}


int NGSPICE::cbSendChar( char* aWhat, int aId, void* aUser )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );

    if( sim->m_reporter )
    {
        // strip stdout/stderr from the line
        if( ( strncasecmp( aWhat, "stdout ", 7 ) == 0 )
                || ( strncasecmp( aWhat, "stderr ", 7 ) == 0 ) )
        {
            aWhat += 7;
        }

        sim->m_reporter->Report( aWhat );
    }

    return 0;
}


int NGSPICE::cbSendStat( char *aWhat, int aId, void* aUser )
{
    return 0;
}


int NGSPICE::cbBGThreadRunning( NG_BOOL aFinished, int aId, void* aUser )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );

    if( sim->m_reporter )
        sim->m_reporter->OnSimStateChange( sim, aFinished ? SIM_IDLE : SIM_RUNNING );

    return 0;
}


int NGSPICE::cbControlledExit( int aStatus, NG_BOOL aImmediate, NG_BOOL aExitOnQuit, int aId,
                               void* aUser )
{
    // Something went wrong, reload the dll
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( aUser );
    sim->m_error = true;

    return 0;
}


void NGSPICE::validate()
{
    if( m_error )
    {
        m_initialized = false;
        init_dll();
    }
}


void NGSPICE::Clean()
{
    Command( "destroy all" );
}


bool NGSPICE::m_initialized = false;
