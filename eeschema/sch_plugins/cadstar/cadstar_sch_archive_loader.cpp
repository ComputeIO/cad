/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_sch_archive_loader.cpp
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#include <sch_plugins/cadstar/cadstar_sch_archive_loader.h>

#include <bus_alias.h>
#include <core/mirror.h>
#include <eda_text.h>
#include <lib_arc.h>
#include <lib_polyline.h>
#include <lib_text.h>
#include <kicad_string.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h> //COMPONENT_ORIENTATION_T
#include <sch_io_mgr.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_text.h>
#include <schematic.h>
#include <trigo.h>
#include <wildcards_and_files_ext.h>


void CADSTAR_SCH_ARCHIVE_LOADER::Load( ::SCHEMATIC* aSchematic, ::SCH_SHEET* aRootSheet,
        SCH_PLUGIN::SCH_PLUGIN_RELEASER* aSchPlugin, const wxFileName& aLibraryFileName )
{
    Parse();

    LONGPOINT designLimit = Assignments.Settings.DesignLimit;

    //Note: can't use getKiCadPoint() due wxPoint being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x * KiCadUnitMultiplier;
    long long designSizeYkicad = (long long) designLimit.y * KiCadUnitMultiplier;

    // Max size limited by the positive dimension of wxPoint (which is an int)
    constexpr long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n"
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ),
                (double) designSizeXkicad / SCH_IU_PER_MM,
                (double) designSizeYkicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM ) );
    }

    // Assume the centre at 0,0 since we are going to be translating the design afterwards anyway
    mDesignCenter = { 0, 0 };

    mSchematic       = aSchematic;
    mRootSheet       = aRootSheet;
    mPlugin          = aSchPlugin;
    mLibraryFileName = aLibraryFileName;

    loadSheets();
    loadHierarchicalSheetPins();
    loadPartsLibrary();
    loadSchematicSymbolInstances();
    loadBusses();
    loadNets();
    loadFigures();
    loadTexts();
    loadDocumentationSymbols();
    loadTextVariables();

    if( Schematic.VariantHierarchy.Variants.size() > 0 )
    {
        wxLogWarning( wxString::Format(
                _( "The CADSTAR design contains variants which has no KiCad equivalent. Only "
                   "the master variant ('%s') was loaded." ),
                Schematic.VariantHierarchy.Variants.at( "V0" ).Name ) );
    }

    if( Schematic.Groups.size() > 0 )
    {
        wxLogWarning(
                _( "The CADSTAR design contains grouped items which has no KiCad equivalent. Any "
                   "grouped items have been ungrouped." ) );
    }

    if( Schematic.ReuseBlocks.size() > 0 )
    {
        wxLogWarning(
                _( "The CADSTAR design contains re-use blocks which has no KiCad equivalent. The "
                   "re-use block information has been discarded during the import." ) );
    }


    // For all sheets, centre all elements and re calculate the page size:
    for( std::pair<LAYER_ID, SCH_SHEET*> sheetPair : mSheetMap )
    {
        SCH_SHEET* sheet = sheetPair.second;

        // Calculate the new sheet size.
        EDA_RECT sheetBoundingBox;

        for( auto item : sheet->GetScreen()->Items() )
        {
            EDA_RECT bbox;

            // Don't use the fields of the components to calculate their bounding box
            // (hidden fields could be very long and artificially enlarge the sheet bounding box)
            if( item->Type() == SCH_COMPONENT_T )
                bbox = static_cast<SCH_COMPONENT*>( item )->GetBodyBoundingBox();
            else
                bbox = item->GetBoundingBox();

            sheetBoundingBox.Merge( bbox );
        }

        auto roundToNearest100mil =
            []( int aNumber ) -> int
            {
                int error = aNumber % Mils2iu( 100 );

                if( abs( error ) > Mils2iu( 50 ) )
                 return aNumber + ( sign(error) * Mils2iu( 100 ) ) - error;
                else
                  return aNumber - error;
            };

        // When exporting to pdf, CADSTAR applies a margin of 3% of the longest dimension (height
        // or width) to all 4 sides (top, bottom, left right). For the import, we are also rounding
        // the margin to the nearest 100mil, ensuring all items remain on the grid
        wxSize targetSheetSize = sheetBoundingBox.GetSize();
        int    longestSide = std::max( targetSheetSize.x, targetSheetSize.y );
        int    margin = ( (double) longestSide * 0.03);
        margin = roundToNearest100mil( margin );
        targetSheetSize.IncBy( margin * 2, margin * 2 );

        // Update page size always
        PAGE_INFO pageInfo   = sheet->GetScreen()->GetPageSettings();
        pageInfo.SetWidthMils( Iu2Mils( targetSheetSize.x ) );
        pageInfo.SetHeightMils( Iu2Mils( targetSheetSize.y ) );

        // Set the new sheet size.
        sheet->GetScreen()->SetPageSettings( pageInfo );

        wxSize  pageSizeIU = sheet->GetScreen()->GetPageSettings().GetSizeIU();
        wxPoint sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
        wxPoint itemsCentre = sheetBoundingBox.Centre();

        // round the translation to nearest 100mil to place it on the grid.
        wxPoint translation = sheetcentre - itemsCentre;
        translation.x = roundToNearest100mil( translation.x );
        translation.y = roundToNearest100mil( translation.y );

        // Translate the items.
        std::vector<SCH_ITEM*> allItems;

        std::copy( sheet->GetScreen()->Items().begin(), sheet->GetScreen()->Items().end(),
                std::back_inserter( allItems ) );

        for( SCH_ITEM* item : allItems )
        {
            item->SetPosition( item->GetPosition() + translation );
            item->ClearFlags();
            sheet->GetScreen()->Update( item );
        }
    }

    wxLogMessage(
            _( "The CADSTAR design has been imported successfully.\n"
               "Please review the import errors and warnings (if any)." ) );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheets()
{
    const std::vector<LAYER_ID>& orphanSheets = findOrphanSheets();
    SCH_SHEET_PATH               rootPath;
    rootPath.push_back( mRootSheet );
    mRootSheet->AddInstance( rootPath.Path() );
    mRootSheet->SetPageNumber( rootPath, wxT( "1" ) );

    if( orphanSheets.size() > 1 )
    {
        int x = 1;
        int y = 1;

        for( LAYER_ID sheetID : orphanSheets )
        {
            wxPoint pos( x * Mils2iu( 1000 ), y * Mils2iu( 1000 ) );
            wxSize  siz( Mils2iu( 1000 ), Mils2iu( 1000 ) );

            loadSheetAndChildSheets( sheetID, pos, siz, rootPath );

            x += 2;

            if( x > 10 ) // start next row
            {
                x = 1;
                y += 2;
            }
        }
    }
    else if( orphanSheets.size() > 0 )
    {
        LAYER_ID rootSheetID = orphanSheets.at( 0 );

        wxFileName loadedFilePath = wxFileName( Filename );

        std::string filename = wxString::Format(
                "%s_%02d", loadedFilePath.GetName(), getSheetNumber( rootSheetID ) )
                                       .ToStdString();
        ReplaceIllegalFileNameChars( &filename );
        filename += wxT( "." ) + KiCadSchematicFileExtension;

        wxFileName fn( filename );
        mRootSheet->GetScreen()->SetFileName( fn.GetFullPath() );

        mSheetMap.insert( { rootSheetID, mRootSheet } );
        loadChildSheets( rootSheetID, rootPath );
    }
    else
    {
        THROW_IO_ERROR( _( "The CADSTAR schematic might be corrupt: there is no root sheet." ) );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadHierarchicalSheetPins()
{
    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK&   block   = blockPair.second;
        LAYER_ID sheetID = "";

        if( block.Type == BLOCK::TYPE::PARENT )
            sheetID = block.LayerID;
        else if( block.Type == BLOCK::TYPE::CHILD )
            sheetID = block.AssocLayerID;
        else
            continue;

        if( mSheetMap.find( sheetID ) != mSheetMap.end() )
        {
            SCH_SHEET* sheet = mSheetMap.at( sheetID );

            for( std::pair<TERMINAL_ID, TERMINAL> termPair : block.Terminals )
            {
                TERMINAL term = termPair.second;
                wxString name = "YOU SHOULDN'T SEE THIS TEXT. THIS IS A BUG.";

                SCH_HIERLABEL* sheetPin = nullptr;

                if( block.Type == BLOCK::TYPE::PARENT )
                    sheetPin = new SCH_HIERLABEL();
                else if( block.Type == BLOCK::TYPE::CHILD )
                    sheetPin = new SCH_SHEET_PIN( sheet );

                sheetPin->SetText( name );
                sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );
                sheetPin->SetLabelSpinStyle( getSpinStyle( term.OrientAngle, false ) );
                sheetPin->SetPosition( getKiCadPoint( term.Position ) );

                if( sheetPin->Type() == SCH_SHEET_PIN_T )
                    sheet->AddPin( (SCH_SHEET_PIN*) sheetPin );
                else
                    sheet->GetScreen()->Append( sheetPin );

                BLOCK_PIN_ID blockPinID = std::make_pair( block.ID, term.ID );
                mSheetPinMap.insert( { blockPinID, sheetPin } );
            }
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadPartsLibrary()
{
    for( std::pair<PART_ID, PART> partPair : Parts.PartDefinitions )
    {
        PART_ID key  = partPair.first;
        PART    part = partPair.second;

        if( part.Definition.GateSymbols.size() == 0 )
            continue;

        LIB_PART* kiPart = new LIB_PART( part.Name );

        kiPart->SetUnitCount( part.Definition.GateSymbols.size() );

        for( std::pair<GATE_ID, PART::DEFINITION::GATE> gatePair : part.Definition.GateSymbols )
        {
            GATE_ID                gateID   = gatePair.first;
            PART::DEFINITION::GATE gate     = gatePair.second;
            SYMDEF_ID              symbolID = getSymDefFromName( gate.Name, gate.Alternate );

            if( symbolID.IsEmpty() )
            {
                wxLogWarning( wxString::Format(
                        _( "Part definition '%s' references symbol '%s' (alternate '%s') "
                           "which could not be found in the symbol library. The part has not "
                           "been loaded into the KiCad library." ),
                        part.Name, gate.Name, gate.Alternate ) );

                continue;
            }

            loadSymDefIntoLibrary( symbolID, &part, gateID, kiPart );
        }

        ( *mPlugin )->SaveSymbol( mLibraryFileName.GetFullPath(), kiPart );

        LIB_PART* loadedPart =
                ( *mPlugin )->LoadSymbol( mLibraryFileName.GetFullPath(), kiPart->GetName() );

        mPartMap.insert( { key, loadedPart } );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSchematicSymbolInstances()
{
    for( std::pair<SYMBOL_ID, SYMBOL> symPair : Schematic.Symbols )
    {
        SYMBOL sym = symPair.second;

        if( !sym.VariantID.empty() && sym.VariantParentSymbolID != sym.ID )
            continue; // Only load master Variant

        if( sym.IsComponent )
        {
            if( mPartMap.find( sym.PartRef.RefID ) == mPartMap.end() )
            {
                wxLogError( wxString::Format(
                        _( "Symbol '%s' references part '%s' which could not be found "
                           "in the library. The symbol was not loaded" ),
                        sym.ComponentRef.Designator, sym.PartRef.RefID ) );

                continue;
            }

            LIB_PART* kiPart = mPartMap.at( sym.PartRef.RefID );
            double    symOrientDeciDeg = 0.0;

            SCH_COMPONENT* component = loadSchematicSymbol( sym, *kiPart, symOrientDeciDeg );

            SCH_FIELD* refField = component->GetField( REFERENCE_FIELD );

            sym.ComponentRef.Designator.Replace( wxT( "\n" ), wxT( "\\n" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\r" ), wxT( "\\r" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\t" ), wxT( "\\t" ) );
            sym.ComponentRef.Designator.Replace( wxT( " " ), wxT( "_" ) );

            refField->SetText( sym.ComponentRef.Designator );
            loadSymbolFieldAttribute( sym.ComponentRef.AttrLoc, symOrientDeciDeg,
                                      sym.Mirror, refField );

            if( sym.HasPartRef )
            {
                SCH_FIELD* partField = component->GetField( FIELD1 );

                if( !partField )
                {
                    component->AddField(
                            SCH_FIELD( wxPoint(), FIELD1, component, wxT( "Part Name" ) ) );

                    partField = component->GetField( FIELD1 );
                }

                wxString partname = getPart( sym.PartRef.RefID ).Name;
                partname.Replace( wxT( "\n" ), wxT( "\\n" ) );
                partname.Replace( wxT( "\r" ), wxT( "\\r" ) );
                partname.Replace( wxT( "\t" ), wxT( "\\t" ) );
                partField->SetText( partname );

                loadSymbolFieldAttribute( sym.PartRef.AttrLoc, symOrientDeciDeg,
                                          sym.Mirror, partField );

                partField->SetVisible( SymbolPartNameColor.IsVisible );
            }

            int fieldIdx = FIELD1;

            for( auto attr : sym.AttributeValues )
            {
                ATTRIBUTE_VALUE attrVal = attr.second;

                if( attrVal.HasLocation )
                {
                    //SCH_FIELD* attrField = getFieldByName( component );
                    wxString attrName = getAttributeName( attrVal.AttributeID );

                    SCH_FIELD* attrField = component->FindField( attrName );

                    if( !attrField )
                    {
                        component->AddField(
                                SCH_FIELD( wxPoint(), ++fieldIdx, component, attrName ) );

                        attrField = component->GetField( fieldIdx );
                    }

                    attrVal.Value.Replace( wxT( "\n" ), wxT( "\\n" ) );
                    attrVal.Value.Replace( wxT( "\r" ), wxT( "\\r" ) );
                    attrVal.Value.Replace( wxT( "\t" ), wxT( "\\t" ) );
                    attrField->SetText( attrVal.Value );

                    loadSymbolFieldAttribute( attrVal.AttributeLocation, symOrientDeciDeg,
                                              sym.Mirror, attrField );
                    attrField->SetVisible( isAttributeVisible( attrVal.AttributeID ) );
                }
            }
        }
        else if( sym.IsSymbolVariant )
        {
            if( Library.SymbolDefinitions.find( sym.SymdefID ) == Library.SymbolDefinitions.end() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Symbol ID '%s' references library symbol '%s' which could not be "
                           "found in the library. Did you export all items of the design?" ),
                        sym.ID, sym.PartRef.RefID ) );
            }

            SYMDEF_SCM libSymDef = Library.SymbolDefinitions.at( sym.SymdefID );

            if( libSymDef.Terminals.size() != 1 )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Symbol ID '%s' is a signal reference or global signal but it has too "
                           "many pins. The expected number of pins is 1 but %d were found." ),
                        sym.ID, libSymDef.Terminals.size() ) );
            }

            if( sym.SymbolVariant.Type == SYMBOLVARIANT::TYPE::GLOBALSIGNAL )
            {
                SYMDEF_ID symID  = sym.SymdefID;
                LIB_PART* kiPart = nullptr;
                //KiCad requires parts to be named the same as the net:
                wxString partName = sym.SymbolVariant.Reference;

                partName = LIB_ID::FixIllegalChars( partName );

                if( mPowerSymLibMap.find( symID ) == mPowerSymLibMap.end()
                        || mPowerSymLibMap.at( symID )->GetName() != partName )
                {
                    kiPart = new LIB_PART( partName );
                    kiPart->SetPower();
                    loadSymDefIntoLibrary( symID, nullptr, "A", kiPart );

                    kiPart->GetValueField().SetText( partName );
                    SYMDEF_SCM symbolDef = Library.SymbolDefinitions.at( symID );

                    if( symbolDef.TextLocations.find( SIGNALNAME_ORIGIN_ATTRID )
                            != symbolDef.TextLocations.end() )
                    {
                        TEXT_LOCATION signameOrigin =
                                symbolDef.TextLocations.at( SIGNALNAME_ORIGIN_ATTRID );
                        kiPart->GetValueField().SetPosition(
                                getKiCadLibraryPoint( signameOrigin.Position, symbolDef.Origin ) );
                        kiPart->GetValueField().SetVisible( true );
                    }
                    else
                    {
                        kiPart->GetValueField().SetVisible( false );
                    }

                    kiPart->GetReferenceField().SetText( "#PWR" );
                    kiPart->GetReferenceField().SetVisible( false );
                    ( *mPlugin )->SaveSymbol( mLibraryFileName.GetFullPath(), kiPart );
                    mPowerSymLibMap.insert( { symID, kiPart } );
                }
                else
                {
                    kiPart = mPowerSymLibMap.at( symID );
                }

                double compOrientationTenthDegree = 0.0;

                SCH_COMPONENT* component =
                        loadSchematicSymbol( sym, *kiPart, compOrientationTenthDegree );

                mPowerSymMap.insert( { sym.ID, component } );
            }
            else if( sym.SymbolVariant.Type == SYMBOLVARIANT::TYPE::SIGNALREF )
            {
                // There should only be one pin and we'll use that to set the position
                TERMINAL& symbolTerminal = libSymDef.Terminals.begin()->second;
                wxPoint   terminalPosOffset = symbolTerminal.Position - libSymDef.Origin;
                double    rotateDeciDegree = getAngleTenthDegree( sym.OrientAngle );

                if( sym.Mirror )
                    rotateDeciDegree += 1800.0;

                RotatePoint( &terminalPosOffset, -rotateDeciDegree );

                SCH_GLOBALLABEL* netLabel = new SCH_GLOBALLABEL;
                netLabel->SetPosition( getKiCadPoint( sym.Origin + terminalPosOffset ) );
                netLabel->SetText( "YOU SHOULDN'T SEE THIS TEXT - PLEASE REPORT THIS BUG" );
                netLabel->SetTextSize( wxSize( Mils2iu( 50 ), Mils2iu( 50 ) ) );
                netLabel->SetLabelSpinStyle( getSpinStyle( sym.OrientAngle, sym.Mirror ) );

                if( libSymDef.Alternate.Lower().Contains( "in" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );
                else if( libSymDef.Alternate.Lower().Contains( "bi" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );
                else if( libSymDef.Alternate.Lower().Contains( "out" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );
                else
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );

                mSheetMap.at( sym.LayerID )->GetScreen()->Append( netLabel );
                mGlobLabelMap.insert( { sym.ID, netLabel } );
            }
            else
            {
                wxASSERT_MSG( false, "Unkown Symbol Variant." );
            }
        }
        else
        {
            wxLogError( wxString::Format(
                    _( "Symbol ID '%s' is of an unknown type. It is neither a component or a "
                       "net power / symbol. The symbol was not loaded." ),
                    sym.ID ) );
        }

        if( sym.ScaleRatioDenominator != 1 || sym.ScaleRatioNumerator != 1 )
        {
            wxString symbolName = sym.ComponentRef.Designator;

            if( symbolName.empty() )
                symbolName = wxString::Format( "ID: %s", sym.ID);

            wxLogError( wxString::Format(
                    _( "Symbol '%s' is scaled in the original CADSTAR schematic but this is not"
                       " supported in KiCad. The symbol was loaded with 1:1 scale and may require "
                       "manual fixing." ),
                    symbolName, sym.PartRef.RefID ) );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadBusses()
{
    for( std::pair<BUS_ID, BUS> busPair : Schematic.Buses )
    {
        BUS    bus     = busPair.second;
        bool   firstPt = true;
        VERTEX last;

        if( bus.LayerID != wxT( "NO_SHEET" ) )
        {
            SCH_SCREEN*                screen     = mSheetMap.at( bus.LayerID )->GetScreen();
            std::shared_ptr<BUS_ALIAS> kiBusAlias = std::make_shared<BUS_ALIAS>();

            kiBusAlias->SetName( bus.Name );
            kiBusAlias->SetParent( screen );
            screen->AddBusAlias( kiBusAlias );
            mBusesMap.insert( { bus.ID, kiBusAlias } );

            SCH_LABEL* label = new SCH_LABEL();
            label->SetText( wxT( "{" ) + bus.Name + wxT( "}" ) );
            label->SetVisible( true );
            screen->Append( label );

            SHAPE_LINE_CHAIN busLineChain; // to compute nearest segment to bus label

            for( const VERTEX& cur : bus.Shape.Vertices )
            {
                busLineChain.Append( getKiCadPoint( cur.End ) );

                if( firstPt )
                {
                    last    = cur;
                    firstPt = false;

                    if( !bus.HasBusLabel )
                    {
                        // Add a bus label on the starting point if the original CADSTAR design
                        // does not have an explicit label
                        label->SetPosition( getKiCadPoint( last.End ) );
                    }

                    continue;
                }


                SCH_LINE* kiBus = new SCH_LINE();

                kiBus->SetStartPoint( getKiCadPoint( last.End ) );
                kiBus->SetEndPoint( getKiCadPoint( cur.End ) );
                kiBus->SetLayer( LAYER_BUS );
                kiBus->SetLineWidth( getLineThickness( bus.LineCodeID ) );
                screen->Append( kiBus );

                last = cur;
            }

            if( bus.HasBusLabel )
            {
                //lets find the closest point in the busline to the label
                VECTOR2I busLabelLoc = getKiCadPoint( bus.BusLabel.Position );
                wxPoint  nearestPt   = (wxPoint) busLineChain.NearestPoint( busLabelLoc );

                label->SetPosition( nearestPt );
                applyTextSettings( bus.BusLabel.TextCodeID, bus.BusLabel.Alignment,
                        bus.BusLabel.Justification, label );
            }
        }

    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadNets()
{
    for( std::pair<NET_ID, NET_SCH> netPair : Schematic.Nets )
    {
        NET_SCH                             net     = netPair.second;
        wxString                            netName = net.Name;
        std::map<NETELEMENT_ID, SCH_LABEL*> netlabels;

        if( netName.IsEmpty() )
            netName = wxString::Format( "$%ld", net.SignalNum );


        for( std::pair<NETELEMENT_ID, NET_SCH::SYM_TERM> terminalPair : net.Terminals )
        {
            NET_SCH::SYM_TERM netTerm = terminalPair.second;

            if( mPowerSymMap.find( netTerm.SymbolID ) != mPowerSymMap.end() )
            {
                SCH_FIELD* val = mPowerSymMap.at( netTerm.SymbolID )->GetField( VALUE_FIELD );
                val->SetText( netName );
                val->SetBold( false );
                val->SetVisible( false );

                if( netTerm.HasNetLabel )
                {
                    val->SetVisible( true );
                    val->SetPosition( getKiCadPoint( netTerm.NetLabel.Position ) );
                    val->SetTextAngle( getAngleTenthDegree( netTerm.NetLabel.OrientAngle ) );

                    applyTextSettings( netTerm.NetLabel.TextCodeID, netTerm.NetLabel.Alignment,
                                       netTerm.NetLabel.Justification, val );
                }
            }
            else if( mGlobLabelMap.find( netTerm.SymbolID ) != mGlobLabelMap.end() )
            {
                mGlobLabelMap.at( netTerm.SymbolID )->SetText( netName );
            }
        }

        auto getHierarchicalLabel = [&]( NETELEMENT_ID aNode ) -> SCH_HIERLABEL*
                                    {
                                        if( aNode.Contains( "BLKT" ) )
                                        {
                                            NET_SCH::BLOCK_TERM blockTerm = net.BlockTerminals.at( aNode );
                                            BLOCK_PIN_ID blockPinID = std::make_pair( blockTerm.BlockID, blockTerm.TerminalID );

                                            if( mSheetPinMap.find( blockPinID )
                                                    != mSheetPinMap.end() )
                                            {
                                                return mSheetPinMap.at( blockPinID );
                                            }
                                        }

                                        return nullptr;
                                    };

        //Add net name to all hierarchical pins (block terminals in CADSTAR)
        for( std::pair<NETELEMENT_ID, NET_SCH::BLOCK_TERM> blockPair : net.BlockTerminals )
        {
            SCH_HIERLABEL* label = getHierarchicalLabel( blockPair.first );

            if(label)
                label->SetText( netName );
        }

        // Load all bus entries and add net label if required
        for( std::pair<NETELEMENT_ID, NET_SCH::BUS_TERM> busPair : net.BusTerminals )
        {
            NET_SCH::BUS_TERM busTerm = busPair.second;
            BUS               bus     = Schematic.Buses.at( busTerm.BusID );

            if( !mBusesMap.at( bus.ID )->Contains( netName ) )
                mBusesMap.at( bus.ID )->AddMember( netName );

            SCH_BUS_WIRE_ENTRY* busEntry =
                    new SCH_BUS_WIRE_ENTRY( getKiCadPoint( busTerm.FirstPoint ), false );

            wxPoint size =
                    getKiCadPoint( busTerm.SecondPoint ) - getKiCadPoint( busTerm.FirstPoint );
            busEntry->SetSize( wxSize( size.x, size.y ) );

            mSheetMap.at( bus.LayerID )->GetScreen()->Append( busEntry );

            if( busTerm.HasNetLabel )
            {
                SCH_LABEL* label = new SCH_LABEL();
                label->SetText( netName );
                label->SetPosition( getKiCadPoint( busTerm.SecondPoint ) );
                label->SetVisible( true );

                applyTextSettings( busTerm.NetLabel.TextCodeID, busTerm.NetLabel.Alignment,
                                   busTerm.NetLabel.Justification, label );

                netlabels.insert( { busTerm.ID, label } );

                mSheetMap.at( bus.LayerID )->GetScreen()->Append( label );
            }
        }


        for( std::pair<NETELEMENT_ID, NET_SCH::DANGLER> danglerPair : net.Danglers )
        {
            NET_SCH::DANGLER dangler = danglerPair.second;

            SCH_LABEL* label = new SCH_LABEL();
            label->SetText( netName );
            label->SetPosition( getKiCadPoint( dangler.Position ) );
            label->SetVisible( true );
            netlabels.insert( { dangler.ID, label } );

            mSheetMap.at( dangler.LayerID )->GetScreen()->Append( label );
        }


        for( NET_SCH::CONNECTION_SCH conn : net.Connections )
        {
            if( conn.LayerID == wxT( "NO_SHEET" ) )
                continue; // No point loading virtual connections. KiCad handles that internally

            POINT start = getLocationOfNetElement( net, conn.StartNode );
            POINT end = getLocationOfNetElement( net, conn.EndNode );

            if( start.x == UNDEFINED_VALUE || end.x == UNDEFINED_VALUE )
                continue;

            // Connections in CADSTAR are always implied between components even if the route
            // doesn't start and end exactly at the connection points
            if( conn.Path.size() < 1 || conn.Path.front() != start )
                conn.Path.insert( conn.Path.begin(), start );

            if( conn.Path.size() < 2 || conn.Path.back() != end )
                conn.Path.push_back( end );

            bool      firstPt  = true;
            bool      secondPt = false;
            wxPoint   last;
            SCH_LINE* wire = nullptr;

            SHAPE_LINE_CHAIN wireChain; // Create a temp. line chain representing the connection

            for( POINT pt : conn.Path )
            {
                wireChain.Append( getKiCadPoint( pt ) );
            }

            // AUTO-FIX SHEET PINS
            //--------------------
            // KiCad constrains the sheet pin on the edge of the sheet object whereas in
            // CADSTAR it can be anywhere. Let's find the intersection of the wires with the sheet
            // and place the hierarchical
            std::vector<NETELEMENT_ID> nodes;
            nodes.push_back( conn.StartNode );
            nodes.push_back( conn.EndNode );

            for( NETELEMENT_ID node : nodes )
            {
                SCH_HIERLABEL* sheetPin = getHierarchicalLabel( node );

                if( sheetPin )
                {
                    if( sheetPin->Type() == SCH_SHEET_PIN_T
                            && SCH_SHEET::ClassOf( sheetPin->GetParent() ) )
                    {
                        SCH_SHEET* parentSheet   = static_cast<SCH_SHEET*>( sheetPin->GetParent() );
                        wxSize     sheetSize     = parentSheet->GetSize();
                        wxPoint    sheetPosition = parentSheet->GetPosition();

                        int leftSide  = sheetPosition.x;
                        int rightSide = sheetPosition.x + sheetSize.x;
                        int topSide   = sheetPosition.y;
                        int botSide   = sheetPosition.y + sheetSize.y;

                        SHAPE_LINE_CHAIN sheetEdge;

                        sheetEdge.Append( leftSide, topSide );
                        sheetEdge.Append( rightSide, topSide );
                        sheetEdge.Append( rightSide, botSide );
                        sheetEdge.Append( leftSide, botSide );
                        sheetEdge.Append( leftSide, topSide );

                        SHAPE_LINE_CHAIN::INTERSECTIONS wireToSheetIntersects;

                        if( !wireChain.Intersect( sheetEdge, wireToSheetIntersects ) )
                        {
                            // The block terminal is outside the block shape in the original
                            // CADSTAR design. Since KiCad's Sheet Pin will already be constrained
                            // on the edge, we will simply join to it with a straight line.
                            if( node == conn.StartNode )
                                wireChain = wireChain.Reverse();

                            wireChain.Append( sheetPin->GetPosition() );

                            if( node == conn.StartNode )
                                wireChain = wireChain.Reverse();
                        }
                        else
                        {
                            // The block terminal is either inside or on the shape edge. Lets use the
                            // first interection point
                            VECTOR2I intsctPt   = wireToSheetIntersects.at( 0 ).p;
                            int      intsctIndx = wireChain.FindSegment( intsctPt );
                            wxASSERT_MSG( intsctIndx != -1, "Can't find intersecting segment" );

                            if( node == conn.StartNode )
                                wireChain.Replace( 0, intsctIndx, intsctPt );
                            else
                                wireChain.Replace( intsctIndx + 1, /*end index*/ -1, intsctPt );

                            sheetPin->SetPosition( (wxPoint) intsctPt );
                        }
                    }
                }
            }

            // Now we can load the wires
            for( const VECTOR2I& pt : wireChain.CPoints() )
            {
                if( firstPt )
                {
                    last     = (wxPoint) pt;
                    firstPt  = false;
                    secondPt = true;
                    continue;
                }

                if( secondPt )
                {
                    secondPt = false;


                    wxPoint          kiLast           = last;
                    wxPoint          kiCurrent        = (wxPoint) pt;
                    double           wireangleDeciDeg = getPolarAngle( kiLast - kiCurrent );
                    LABEL_SPIN_STYLE spin             = getSpinStyleDeciDeg( wireangleDeciDeg );

                    if( netlabels.find( conn.StartNode ) != netlabels.end() )
                    {
                        netlabels.at( conn.StartNode )->SetLabelSpinStyle( spin );
                    }

                    SCH_HIERLABEL* sheetPin = getHierarchicalLabel( conn.StartNode );

                    if( sheetPin )
                        sheetPin->SetLabelSpinStyle( spin );
                }

                wire = new SCH_LINE();

                wire->SetStartPoint( last );
                wire->SetEndPoint( (wxPoint) pt );
                wire->SetLayer( LAYER_WIRE );

                if( !conn.ConnectionLineCode.IsEmpty() )
                    wire->SetLineWidth( getLineThickness( conn.ConnectionLineCode ) );

                last = (wxPoint) pt;

                mSheetMap.at( conn.LayerID )->GetScreen()->Append( wire );
            }

            //Fix labels on the end wire
            if( wire )
            {
                wxPoint          kiLast           = wire->GetEndPoint();
                wxPoint          kiCurrent        = wire->GetStartPoint();
                double           wireangleDeciDeg = getPolarAngle( kiLast - kiCurrent );
                LABEL_SPIN_STYLE spin             = getSpinStyleDeciDeg( wireangleDeciDeg );

                if( netlabels.find( conn.EndNode ) != netlabels.end() )
                    netlabels.at( conn.EndNode )->SetLabelSpinStyle( spin );

                SCH_HIERLABEL* sheetPin = getHierarchicalLabel( conn.EndNode );

                if( sheetPin )
                    sheetPin->SetLabelSpinStyle( spin );
            }
        }

        for( std::pair<NETELEMENT_ID, NET_SCH::JUNCTION_SCH> juncPair : net.Junctions )
        {
            NET_SCH::JUNCTION_SCH junc = juncPair.second;

            SCH_JUNCTION* kiJunc = new SCH_JUNCTION();

            kiJunc->SetPosition( getKiCadPoint( junc.Location ) );
            mSheetMap.at( junc.LayerID )->GetScreen()->Append( kiJunc );

            if( junc.HasNetLabel )
            {
                // In CADSTAR the label can be placed anywhere, but in KiCad it has to be placed
                // in the same location as the junction for it to be connected to it.
                SCH_LABEL* label = new SCH_LABEL();
                label->SetText( netName );
                label->SetPosition( getKiCadPoint( junc.Location ) );
                label->SetVisible( true );

                double labelAngleDeciDeg = getAngleTenthDegree( junc.NetLabel.OrientAngle );
                LABEL_SPIN_STYLE spin = getSpinStyleDeciDeg( labelAngleDeciDeg );
                label->SetLabelSpinStyle( spin );

                mSheetMap.at( junc.LayerID )->GetScreen()->Append( label );
            }
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadFigures()
{
    for( std::pair<FIGURE_ID, FIGURE> figPair : Schematic.Figures )
    {
        FIGURE fig = figPair.second;

        loadFigure( fig, fig.LayerID, LAYER_NOTES );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadTexts()
{
    for( std::pair<TEXT_ID, TEXT> textPair : Schematic.Texts )
    {
        TEXT txt = textPair.second;

        SCH_TEXT* kiTxt = getKiCadSchText( txt );
        loadItemOntoKiCadSheet( txt.LayerID, kiTxt );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadDocumentationSymbols()
{
    for( std::pair<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> docSymPair :
            Schematic.DocumentationSymbols )
    {
        DOCUMENTATION_SYMBOL docSym = docSymPair.second;

        if( Library.SymbolDefinitions.find( docSym.SymdefID ) == Library.SymbolDefinitions.end() )
        {
            wxLogError(
                    wxString::Format( _( "Documentation Symbol '%s' refers to symbol definition "
                                         "ID '%s' which does not exist in the library. The symbol "
                                         "was not loaded." ),
                            docSym.ID, docSym.SymdefID ) );
            continue;
        }

        SYMDEF_SCM docSymDef  = Library.SymbolDefinitions.at( docSym.SymdefID );
        wxPoint    moveVector = getKiCadPoint( docSym.Origin ) - getKiCadPoint( docSymDef.Origin );
        double     rotationAngle = getAngleTenthDegree( docSym.OrientAngle );
        double     scalingFactor =
                (double) docSym.ScaleRatioNumerator / (double) docSym.ScaleRatioDenominator;
        wxPoint centreOfTransform = getKiCadPoint( docSymDef.Origin );
        bool    mirrorInvert      = docSym.Mirror;

        for( std::pair<FIGURE_ID, FIGURE> figPair : docSymDef.Figures )
        {
            FIGURE fig = figPair.second;

            loadFigure( fig, docSym.LayerID, LAYER_NOTES, moveVector, rotationAngle, scalingFactor,
                    centreOfTransform, mirrorInvert );
        }

        for( std::pair<TEXT_ID, TEXT> textPair : docSymDef.Texts )
        {
            TEXT txt = textPair.second;

            SCH_TEXT* kiTxt = getKiCadSchText( txt );

            wxPoint newPosition = applyTransform( kiTxt->GetPosition(), moveVector, rotationAngle,
                    scalingFactor, centreOfTransform, mirrorInvert );
            double  newTxtAngle = NormalizeAnglePos( kiTxt->GetTextAngle() + rotationAngle );
            bool    newMirrorStatus = kiTxt->IsMirrored() ? !mirrorInvert : mirrorInvert;
            int     newTxtWidth     = KiROUND( kiTxt->GetTextWidth() * scalingFactor );
            int     newTxtHeight    = KiROUND( kiTxt->GetTextHeight() * scalingFactor );
            int     newTxtThickness = KiROUND( kiTxt->GetTextThickness() * scalingFactor );

            kiTxt->SetPosition( newPosition );
            kiTxt->SetTextAngle( newTxtAngle );
            kiTxt->SetMirrored( newMirrorStatus );
            kiTxt->SetTextWidth( newTxtWidth );
            kiTxt->SetTextHeight( newTxtHeight );
            kiTxt->SetTextThickness( newTxtThickness );

            loadItemOntoKiCadSheet( docSym.LayerID, kiTxt );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadTextVariables()
{
    auto findAndReplaceTextField = [&]( TEXT_FIELD_NAME aField, wxString aValue ) {
        if( mContext.TextFieldToValuesMap.find( aField ) != mContext.TextFieldToValuesMap.end() )
        {
            if( mContext.TextFieldToValuesMap.at( aField ) != aValue )
            {
                mContext.TextFieldToValuesMap.at( aField ) = aValue;
                mContext.InconsistentTextFields.insert( aField );
                return false;
            }
        }
        else
        {
            mContext.TextFieldToValuesMap.insert( { aField, aValue } );
        }

        return true;
    };

    PROJECT* pj = &mSchematic->Prj();

    if( pj )
    {
        std::map<wxString, wxString>& txtVars = pj->GetTextVars();

        // Most of the design text fields can be derived from other elements
        if( Schematic.VariantHierarchy.Variants.size() > 0 )
        {
            VARIANT loadedVar = Schematic.VariantHierarchy.Variants.begin()->second;

            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_NAME, loadedVar.Name );
            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_DESCRIPTION, loadedVar.Description );
        }

        findAndReplaceTextField( TEXT_FIELD_NAME::DESIGN_TITLE, Header.JobTitle );

        for( std::pair<TEXT_FIELD_NAME, wxString> txtvalue : mContext.TextFieldToValuesMap )
        {
            wxString varName  = CadstarToKicadFieldsMap.at( txtvalue.first );
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }

        for( std::pair<wxString, wxString> txtvalue : mContext.FilenamesToTextMap )
        {
            wxString varName  = txtvalue.first;
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }
    }
    else
    {
        wxLogError( _( "Text Variables could not be set as there is no project attached." ) );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymDefIntoLibrary( const SYMDEF_ID& aSymdefID,
        const PART* aCadstarPart, const GATE_ID& aGateID, LIB_PART* aPart )
{
    wxCHECK( Library.SymbolDefinitions.find( aSymdefID ) != Library.SymbolDefinitions.end(), );

    SYMDEF_SCM symbol = Library.SymbolDefinitions.at( aSymdefID );
    int        gateNumber = getKiCadUnitNumberFromGate( aGateID );

    for( std::pair<FIGURE_ID, FIGURE> figPair : symbol.Figures )
    {
        FIGURE fig = figPair.second;

        loadLibrarySymbolShapeVertices( fig.Shape.Vertices, symbol.Origin, aPart, gateNumber );

        for( CUTOUT c : fig.Shape.Cutouts )
        {
            loadLibrarySymbolShapeVertices( c.Vertices, symbol.Origin, aPart, gateNumber );
        }
    }

    TERMINAL_TO_PINNUM_MAP pinNumMap;

    for( std::pair<TERMINAL_ID, TERMINAL> termPair : symbol.Terminals )
    {
        TERMINAL term    = termPair.second;
        wxString pinNum  = wxString::Format( "%ld", term.ID );
        wxString pinName = wxEmptyString;
        LIB_PIN* pin = new LIB_PIN( aPart );

        if( aCadstarPart )
        {
            PART::DEFINITION::PIN csPin = getPartDefinitionPin( *aCadstarPart, aGateID, term.ID );

            pinName = csPin.Label;
            pinNum  = csPin.Name;

            if( pinNum.IsEmpty() )
            {
                if( !csPin.Identifier.IsEmpty() )
                    pinNum = csPin.Identifier;
                else if( csPin.ID == UNDEFINED_VALUE )
                    pinNum = wxString::Format( "%ld", term.ID );
                else
                    pinNum = wxString::Format( "%ld", csPin.ID );
            }

            pin->SetType( getKiCadPinType( csPin.Type ) );

            pinNumMap.insert( { term.ID, pinNum } );
        }
        else
        {
            // If no part is defined, we don't know the pin type. Assume passive pin
            pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        }

        pin->SetPosition( getKiCadLibraryPoint( term.Position, symbol.Origin ) );
        pin->SetLength( 0 ); //CADSTAR Pins are just a point (have no length)
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );
        pin->SetUnit( gateNumber );
        pin->SetNumber( pinNum );
        pin->SetName( pinName );

        int oDeg = (int) NormalizeAngle180( getAngleTenthDegree( term.OrientAngle ) );

        if( oDeg >= -450 && oDeg <= 450 )
            pin->SetOrientation( 'R' ); // 0 degrees
        else if( oDeg >= 450 && oDeg <= 1350 )
            pin->SetOrientation( 'U' ); // 90 degrees
        else if( oDeg >= 1350 || oDeg <= -1350 )
            pin->SetOrientation( 'L' ); // 180 degrees
        else
            pin->SetOrientation( 'D' ); // -90 degrees

        if( aPart->IsPower() )
        {
            pin->SetVisible( false );
            pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            pin->SetName( aPart->GetName() );
        }

        aPart->AddDrawItem( pin );
    }

    if(aCadstarPart)
        mPinNumsMap.insert( { aCadstarPart->ID + aGateID, pinNumMap } );

    for( std::pair<TEXT_ID, TEXT> textPair : symbol.Texts )
    {
        TEXT      csText  = textPair.second;
        LIB_TEXT* libtext = new LIB_TEXT( aPart );
        libtext->SetText( csText.Text );
        libtext->SetUnit( gateNumber );
        libtext->SetPosition( getKiCadLibraryPoint( csText.Position, symbol.Origin ) );
        applyTextSettings( csText.TextCodeID, csText.Alignment, csText.Justification, libtext );
        aPart->AddDrawItem( libtext );
    }

    if( symbol.TextLocations.find( SYMBOL_NAME_ATTRID ) != symbol.TextLocations.end() )
    {
        TEXT_LOCATION textLoc = symbol.TextLocations.at( SYMBOL_NAME_ATTRID );
        LIB_FIELD*    field = &aPart->GetReferenceField();
        applyToLibraryFieldAttribute( textLoc, symbol.Origin, field );
        field->SetUnit( gateNumber );
    }

    // Hide the value field for now (it might get unhidden if an attribute exists in the cadstar
    // design with the text "Value"
    aPart->GetValueField().SetVisible( false );

    if( symbol.TextLocations.find( PART_NAME_ATTRID ) != symbol.TextLocations.end() )
    {
        TEXT_LOCATION textLoc = symbol.TextLocations.at( PART_NAME_ATTRID );
        LIB_FIELD*    field   = aPart->GetField( FIELD1 );

        if( !field )
        {
            field = new LIB_FIELD( aPart, FIELD1 );
            aPart->AddField( field );
        }

        field->SetName( "Part Name" );
        applyToLibraryFieldAttribute( textLoc, symbol.Origin, field );

        if( aCadstarPart )
        {
            wxString partName = aCadstarPart->Name;
            partName.Replace( wxT( "\n" ), wxT( "\\n" ) );
            partName.Replace( wxT( "\r" ), wxT( "\\r" ) );
            partName.Replace( wxT( "\t" ), wxT( "\\t" ) );
            field->SetText( partName );
        }

        field->SetUnit( gateNumber );
        field->SetVisible( SymbolPartNameColor.IsVisible );
    }

    if( aCadstarPart )
    {
        int      fieldIdx = FIELD1;
        wxString footprintRefName = wxEmptyString;
        wxString footprintAlternateName = wxEmptyString;

        auto loadLibraryField =
            [&]( ATTRIBUTE_VALUE& aAttributeVal )
            {
                wxString   attrName = getAttributeName( aAttributeVal.AttributeID );
                LIB_FIELD* attrField = nullptr;

                //Remove invalid field characters
                aAttributeVal.Value.Replace( wxT( "\n" ), wxT( "\\n" ) );
                aAttributeVal.Value.Replace( wxT( "\r" ), wxT( "\\r" ) );
                aAttributeVal.Value.Replace( wxT( "\t" ), wxT( "\\t" ) );

                //TODO: Handle "links": In cadstar a field can be a "link" if its name starts with the
                //characters "Link ". Need to figure out how to convert them to equivalent in KiCad

                if( attrName == wxT( "(PartDefinitionNameStem)" ) )
                {
                    //Space not allowed in Reference field
                    aAttributeVal.Value.Replace( wxT( " " ), "_" );
                    aPart->GetReferenceField().SetText( aAttributeVal.Value );
                    return;
                }
                else if( attrName == wxT( "(PartDescription)" ) )
                {
                    aPart->SetDescription( aAttributeVal.Value );
                    return;
                }
                else if( attrName == wxT( "(PartDefinitionReferenceName)" ) )
                {
                    footprintRefName = aAttributeVal.Value;
                    return;
                }
                else if( attrName == wxT( "(PartDefinitionAlternateName)" ) )
                {
                    footprintAlternateName = aAttributeVal.Value;
                    return;
                }
                else
                {
                    attrField = aPart->FindField( attrName );
                }

                if( !attrField )
                {
                    aPart->AddField( new LIB_FIELD( aPart, ++fieldIdx ) );
                    attrField = aPart->GetField( fieldIdx );
                    attrField->SetName( attrName );
                }

                attrField->SetText( aAttributeVal.Value );
                attrField->SetUnit( gateNumber );

                if( aAttributeVal.HasLocation )
                {
                    // #1 Check if the part itself defined a location for the field
                    applyToLibraryFieldAttribute( aAttributeVal.AttributeLocation, symbol.Origin,
                                                  attrField );
                    attrField->SetVisible( isAttributeVisible( aAttributeVal.AttributeID ) );
                }
                else if( symbol.TextLocations.find( aAttributeVal.AttributeID )
                         != symbol.TextLocations.end() )
                {
                    // #2 Look in the symbol definition: Text locations
                    TEXT_LOCATION symTxtLoc = symbol.TextLocations.at( aAttributeVal.AttributeID );

                    applyToLibraryFieldAttribute( symTxtLoc, symbol.Origin, attrField );
                    attrField->SetVisible( isAttributeVisible( symTxtLoc.AttributeID ) );
                }
                else if( symbol.AttributeValues.find( aAttributeVal.AttributeID )
                         != symbol.AttributeValues.end() )
                {
                    // #3 Look in the symbol definition: Attribute values
                    ATTRIBUTE_VALUE symAttrVal =
                            symbol.AttributeValues.at( aAttributeVal.AttributeID );

                    if( symAttrVal.HasLocation )
                    {
                        applyToLibraryFieldAttribute( symAttrVal.AttributeLocation, symbol.Origin,
                                                      attrField );
                        attrField->SetVisible( isAttributeVisible( symAttrVal.AttributeID ) );
                    }
                    else
                    {
                        attrField->SetVisible( false );
                        applyTextSettings( wxT( "TC1" ), ALIGNMENT::NO_ALIGNMENT,
                                           JUSTIFICATION::LEFT, attrField );
                    }
                }
                else
                {
                    attrField->SetVisible( false );
                    applyTextSettings( wxT( "TC1" ), ALIGNMENT::NO_ALIGNMENT,
                                       JUSTIFICATION::LEFT, attrField );
                }
            };

        // Load all attributes in the Part Definition
        for( std::pair<ATTRIBUTE_ID, ATTRIBUTE_VALUE> attr : aCadstarPart->Definition.AttributeValues )
        {
            ATTRIBUTE_VALUE attrVal = attr.second;
            loadLibraryField( attrVal );
        }

        // Load all attributes in the Part itself.
        for( std::pair<ATTRIBUTE_ID, ATTRIBUTE_VALUE> attr : aCadstarPart->AttributeValues )
        {
            ATTRIBUTE_VALUE attrVal = attr.second;
            loadLibraryField( attrVal );
        }

        wxString      fpNameInLibrary = generateLibName( footprintRefName, footprintAlternateName );
        wxArrayString fpFilters;
        fpFilters.Add( fpNameInLibrary );

        aPart->SetFPFilters( fpFilters );

        // Assume that the PCB footprint library name will be the same as the schematic filename
        wxFileName schFilename( Filename );
        wxString   libName = schFilename.GetName();

        aPart->GetFootprintField().SetText( libName + wxT( ":" ) + fpNameInLibrary );
    }

    if( aCadstarPart && aCadstarPart->Definition.HidePinNames )
    {
        aPart->SetShowPinNames( false );
        aPart->SetShowPinNumbers( false );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadLibrarySymbolShapeVertices(
        const std::vector<VERTEX>& aCadstarVertices, wxPoint aSymbolOrigin, LIB_PART* aPart,
        int aGateNumber )
{
    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG(
            prev->Type == VERTEX_TYPE::POINT, "First vertex should always be a point vertex" );

    for( size_t i = 1; i < aCadstarVertices.size(); i++ )
    {
        cur = &aCadstarVertices.at( i );

        LIB_ITEM* segment    = nullptr;
        bool      cw         = false;
        wxPoint   startPoint = getKiCadLibraryPoint( prev->End, aSymbolOrigin );
        wxPoint   endPoint   = getKiCadLibraryPoint( cur->End, aSymbolOrigin );
        wxPoint   centerPoint;

        if( cur->Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
                || cur->Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        {
            centerPoint = ( startPoint + endPoint ) / 2;
        }
        else
        {
            centerPoint = getKiCadLibraryPoint( cur->Center, aSymbolOrigin );
        }


        switch( cur->Type )
        {
        case VERTEX_TYPE::POINT:
            segment = new LIB_POLYLINE( aPart );
            ( (LIB_POLYLINE*) segment )->AddPoint( startPoint );
            ( (LIB_POLYLINE*) segment )->AddPoint( endPoint );
            break;

        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
            cw = true;
            KI_FALLTHROUGH;

        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
            segment = new LIB_ARC( aPart );

            ( (LIB_ARC*) segment )->SetPosition( centerPoint );

            if( cw )
            {
                ( (LIB_ARC*) segment )->SetStart( endPoint );
                ( (LIB_ARC*) segment )->SetEnd( startPoint );
            }
            else
            {
                ( (LIB_ARC*) segment )->SetStart( startPoint );
                ( (LIB_ARC*) segment )->SetEnd( endPoint );
            }

            ( (LIB_ARC*) segment )->CalcRadiusAngles();
            break;
        }

        segment->SetUnit( aGateNumber );
        aPart->AddDrawItem( segment );

        prev = cur;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyToLibraryFieldAttribute(
        const ATTRIBUTE_LOCATION& aCadstarAttrLoc, wxPoint aSymbolOrigin, LIB_FIELD* aKiCadField )
{
    aKiCadField->SetTextPos( getKiCadLibraryPoint( aCadstarAttrLoc.Position, aSymbolOrigin ) );
    aKiCadField->SetTextAngle( getAngleTenthDegree( aCadstarAttrLoc.OrientAngle ) );
    aKiCadField->SetBold( false );
    aKiCadField->SetVisible( true );

    applyTextSettings( aCadstarAttrLoc.TextCodeID, aCadstarAttrLoc.Alignment,
            aCadstarAttrLoc.Justification, aKiCadField );
}


SCH_COMPONENT* CADSTAR_SCH_ARCHIVE_LOADER::loadSchematicSymbol(
        const SYMBOL& aCadstarSymbol, const LIB_PART& aKiCadPart, double& aComponentOrientationDeciDeg )
{
    LIB_ID  libId( mLibraryFileName.GetName(), aKiCadPart.GetName() );
    int     unit = getKiCadUnitNumberFromGate( aCadstarSymbol.GateID );

    SCH_SHEET_PATH sheetpath;
    SCH_SHEET* kiSheet = mSheetMap.at( aCadstarSymbol.LayerID );
    mRootSheet->LocatePathOfScreen( kiSheet->GetScreen(), &sheetpath );

    SCH_COMPONENT* component = new SCH_COMPONENT( aKiCadPart, libId, &sheetpath, unit );

    if( aCadstarSymbol.IsComponent )
    {
        component->SetRef( &sheetpath, aCadstarSymbol.ComponentRef.Designator );
    }

    component->SetPosition( getKiCadPoint( aCadstarSymbol.Origin ) );

    double compAngleDeciDeg = getAngleTenthDegree( aCadstarSymbol.OrientAngle );
    int compOrientation  = 0;

    if( aCadstarSymbol.Mirror )
    {
        compAngleDeciDeg = -compAngleDeciDeg;
        compOrientation += COMPONENT_ORIENTATION_T::CMP_MIRROR_Y;
    }

    compOrientation += getComponentOrientation( compAngleDeciDeg, aComponentOrientationDeciDeg );

    if( NormalizeAngle180( compAngleDeciDeg ) != NormalizeAngle180( aComponentOrientationDeciDeg ) )
    {
        wxLogError(
                wxString::Format( _( "Symbol '%s' is rotated by an angle of %.1f degrees in the "
                                     "original CADSTAR design but KiCad only supports rotation "
                                     "angles multiples of 90 degrees. The connecting wires will "
                                     "need manual fixing." ),
                        aCadstarSymbol.ComponentRef.Designator, compAngleDeciDeg / 10.0 ) );
    }

    component->SetOrientation( compOrientation );

    if( mSheetMap.find( aCadstarSymbol.LayerID ) == mSheetMap.end() )
    {
        wxLogError(
                wxString::Format( _( "Symbol '%s' references sheet ID '%s' which does not exist in "
                                     "the design. The symbol was not loaded." ),
                        aCadstarSymbol.ComponentRef.Designator, aCadstarSymbol.LayerID ) );

        delete component;
        return nullptr;
    }

    wxString gate = ( aCadstarSymbol.GateID.IsEmpty() ) ? wxT( "A" ) : aCadstarSymbol.GateID;
    wxString partGateIndex = aCadstarSymbol.PartRef.RefID + gate;

    //Handle pin swaps
    if( mPinNumsMap.find( partGateIndex ) != mPinNumsMap.end() )
    {
        TERMINAL_TO_PINNUM_MAP termNumMap = mPinNumsMap.at( partGateIndex );

        std::map<wxString, LIB_PIN*> pinNumToLibPinMap;

        for( auto& term : termNumMap )
        {
            wxString pinNum = term.second;
            pinNumToLibPinMap.insert( { pinNum, component->GetPartRef()->GetPin( term.second ) } );
        }

        auto replacePinNumber = [&]( wxString aOldPinNum, wxString aNewPinNum )
                                {
                                    if( aOldPinNum == aNewPinNum )
                                        return;

                                    LIB_PIN* libpin = pinNumToLibPinMap.at( aOldPinNum );
                                    libpin->SetNumber( aNewPinNum );
                                };

        //Older versions of Cadstar used pin numbers
        for( auto& pinPair : aCadstarSymbol.PinNumbers )
        {
            SYMBOL::PIN_NUM pin = pinPair.second;

            replacePinNumber( termNumMap.at( pin.TerminalID ),
                              wxString::Format( "%ld", pin.PinNum ) );
        }

        //Newer versions of Cadstar use pin names
        for( auto& pinPair : aCadstarSymbol.PinNames )
        {
            SYMPINNAME_LABEL pin = pinPair.second;
            replacePinNumber( termNumMap.at( pin.TerminalID ), pin.NameOrLabel );
        }

        component->UpdatePins();
    }

    kiSheet->GetScreen()->Append( component );

    return component;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymbolFieldAttribute(
        const ATTRIBUTE_LOCATION& aCadstarAttrLoc, const double& aComponentOrientationDeciDeg,
        bool aIsMirrored, SCH_FIELD* aKiCadField )
{
    aKiCadField->SetPosition( getKiCadPoint( aCadstarAttrLoc.Position ) );
    aKiCadField->SetTextAngle( getAngleTenthDegree( aCadstarAttrLoc.OrientAngle )
                               - aComponentOrientationDeciDeg );
    aKiCadField->SetBold( false );
    aKiCadField->SetVisible( true );

    ALIGNMENT fieldAlignment = aCadstarAttrLoc.Alignment;
    JUSTIFICATION fieldJustification = aCadstarAttrLoc.Justification;

    // KiCad mirrors the justification and alignment when the component is mirrored but CADSTAR
    // specifies it post-mirroring
    if( aIsMirrored )
    {
        switch( fieldAlignment )
        {
        // Change left to right:
        case ALIGNMENT::NO_ALIGNMENT:
        case ALIGNMENT::BOTTOMLEFT:   fieldAlignment = ALIGNMENT::BOTTOMRIGHT;   break;
        case ALIGNMENT::CENTERLEFT:   fieldAlignment = ALIGNMENT::CENTERRIGHT;   break;
        case ALIGNMENT::TOPLEFT:      fieldAlignment = ALIGNMENT::TOPRIGHT;      break;
        //Change right to left:
        case ALIGNMENT::BOTTOMRIGHT:  fieldAlignment = ALIGNMENT::BOTTOMLEFT;       break;
        case ALIGNMENT::CENTERRIGHT:  fieldAlignment = ALIGNMENT::CENTERLEFT;    break;
        case ALIGNMENT::TOPRIGHT:     fieldAlignment = ALIGNMENT::TOPLEFT;       break;
        // Center alignment does not mirror:
        case ALIGNMENT::BOTTOMCENTER:
        case ALIGNMENT::CENTERCENTER:
        case ALIGNMENT::TOPCENTER:                                               break;
        }

        switch( fieldJustification )
        {
        case JUSTIFICATION::LEFT:    fieldJustification = JUSTIFICATION::RIGHT;  break;
        case JUSTIFICATION::RIGHT:   fieldJustification = JUSTIFICATION::LEFT;   break;
        case JUSTIFICATION::CENTER:                                              break;
        }
    }


    applyTextSettings( aCadstarAttrLoc.TextCodeID, fieldAlignment, fieldJustification,
                       aKiCadField );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getComponentOrientation(
        double aOrientAngleDeciDeg, double& aReturnedOrientationDeciDeg )
{
    int compOrientation = COMPONENT_ORIENTATION_T::CMP_ORIENT_0;

    int oDeg = (int) NormalizeAngle180( aOrientAngleDeciDeg );

    if( oDeg >= -450 && oDeg <= 450 )
    {
        compOrientation             = COMPONENT_ORIENTATION_T::CMP_ORIENT_0;
        aReturnedOrientationDeciDeg = 0.0;
    }
    else if( oDeg >= 450 && oDeg <= 1350 )
    {
        compOrientation             = COMPONENT_ORIENTATION_T::CMP_ORIENT_90;
        aReturnedOrientationDeciDeg = 900.0;
    }
    else if( oDeg >= 1350 || oDeg <= -1350 )
    {
        compOrientation             = COMPONENT_ORIENTATION_T::CMP_ORIENT_180;
        aReturnedOrientationDeciDeg = 1800.0;
    }
    else
    {
        compOrientation             = COMPONENT_ORIENTATION_T::CMP_ORIENT_270;
        aReturnedOrientationDeciDeg = 2700.0;
    }

    return compOrientation;
}


CADSTAR_SCH_ARCHIVE_LOADER::POINT CADSTAR_SCH_ARCHIVE_LOADER::getLocationOfNetElement(
        const NET_SCH& aNet, const NETELEMENT_ID& aNetElementID )
{
    // clang-format off
    auto logUnknownNetElementError =
        [&]()
        {
            wxLogError( wxString::Format( _(
                "Net %s references unknown net element %s. The net was "
                "not properly loaded and may require manual fixing." ),
                    getNetName( aNet ), aNetElementID ) );

            return POINT();
        };
    // clang-format on

    if( aNetElementID.Contains( "J" ) ) // Junction
    {
        if( aNet.Junctions.find( aNetElementID ) == aNet.Junctions.end() )
            return logUnknownNetElementError();

        return aNet.Junctions.at( aNetElementID ).Location;
    }
    else if( aNetElementID.Contains( "P" ) ) // Terminal/Pin of a symbol
    {
        if( aNet.Terminals.find( aNetElementID ) == aNet.Terminals.end() )
            return logUnknownNetElementError();

        SYMBOL_ID   symid  = aNet.Terminals.at( aNetElementID ).SymbolID;
        TERMINAL_ID termid = aNet.Terminals.at( aNetElementID ).TerminalID;

        if( Schematic.Symbols.find( symid ) == Schematic.Symbols.end() )
            return logUnknownNetElementError();

        SYMBOL    sym          = Schematic.Symbols.at( symid );
        SYMDEF_ID symdefid     = sym.SymdefID;
        wxPoint   symbolOrigin = sym.Origin;

        if( Library.SymbolDefinitions.find( symdefid ) == Library.SymbolDefinitions.end() )
            return logUnknownNetElementError();

        wxPoint libpinPosition =
                Library.SymbolDefinitions.at( symdefid ).Terminals.at( termid ).Position;
        wxPoint libOrigin   = Library.SymbolDefinitions.at( symdefid ).Origin;
        wxPoint pinOffset   = libpinPosition - libOrigin;
        wxPoint pinPosition = symbolOrigin + pinOffset;

        double compAngleDeciDeg = getAngleTenthDegree( sym.OrientAngle );

        if( sym.Mirror )
            pinPosition.x = ( 2 * symbolOrigin.x ) - pinPosition.x;

        double adjustedOrientationDecideg;
        getComponentOrientation( compAngleDeciDeg, adjustedOrientationDecideg );

        RotatePoint( &pinPosition, symbolOrigin, -adjustedOrientationDecideg );

        POINT retval;
        retval.x = pinPosition.x;
        retval.y = pinPosition.y;

        return retval;
    }
    else if( aNetElementID.Contains( "BT" ) ) // Bus Terminal
    {
        if( aNet.BusTerminals.find( aNetElementID ) == aNet.BusTerminals.end() )
            return logUnknownNetElementError();

        return aNet.BusTerminals.at( aNetElementID ).SecondPoint;
    }
    else if( aNetElementID.Contains( "BLKT" ) ) // Block Terminal (sheet hierarchy connection)
    {
        if( aNet.BlockTerminals.find( aNetElementID ) == aNet.BlockTerminals.end() )
            return logUnknownNetElementError();

        BLOCK_ID    blockid = aNet.BlockTerminals.at( aNetElementID ).BlockID;
        TERMINAL_ID termid  = aNet.BlockTerminals.at( aNetElementID ).TerminalID;

        if( Schematic.Blocks.find( blockid ) == Schematic.Blocks.end() )
            return logUnknownNetElementError();

        return Schematic.Blocks.at( blockid ).Terminals.at( termid ).Position;
    }
    else if( aNetElementID.Contains( "D" ) ) // Dangler
    {
        if( aNet.Danglers.find( aNetElementID ) == aNet.Danglers.end() )
            return logUnknownNetElementError();

        return aNet.Danglers.at( aNetElementID ).Position;
    }
    else
    {
        return logUnknownNetElementError();
    }

    return POINT();
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getNetName( const NET_SCH& aNet )
{
    wxString netname = aNet.Name;

    if( netname.IsEmpty() )
        netname = wxString::Format( "$%ld", aNet.SignalNum );

    return netname;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadGraphicStaightSegment( const wxPoint& aStartPoint,
        const wxPoint& aEndPoint, const LINECODE_ID& aCadstarLineCodeID,
        const LAYER_ID& aCadstarSheetID, const SCH_LAYER_ID& aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    SCH_LINE* segment = new SCH_LINE();

    segment->SetLayer( aKiCadSchLayerID );
    segment->SetLineWidth( KiROUND( getLineThickness( aCadstarLineCodeID ) * aScalingFactor ) );
    segment->SetLineStyle( getLineStyle( aCadstarLineCodeID ) );

    //Apply transforms
    wxPoint startPoint = applyTransform( aStartPoint, aMoveVector, aRotationAngleDeciDeg, aScalingFactor,
            aTransformCentre, aMirrorInvert );
    wxPoint endPoint = applyTransform( aEndPoint, aMoveVector, aRotationAngleDeciDeg, aScalingFactor,
            aTransformCentre, aMirrorInvert );

    segment->SetStartPoint( startPoint );
    segment->SetEndPoint( endPoint );

    loadItemOntoKiCadSheet( aCadstarSheetID, segment );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
        LINECODE_ID aCadstarLineCodeID, LAYER_ID aCadstarSheetID, SCH_LAYER_ID aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG(
            prev->Type == VERTEX_TYPE::POINT, "First vertex should always be a point vertex" );

    for( size_t ii = 1; ii < aCadstarVertices.size(); ii++ )
    {
        cur = &aCadstarVertices.at( ii );

        wxPoint   startPoint  = getKiCadPoint( prev->End );
        wxPoint   endPoint    = getKiCadPoint( cur->End );
        wxPoint   centerPoint = getKiCadPoint( cur->Center );
        bool      cw          = false;

        if( cur->Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
                || cur->Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        {
            centerPoint = ( startPoint + endPoint ) / 2;
        }

        switch( cur->Type )
        {
        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
            cw = true;
            KI_FALLTHROUGH;
        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
        {
            double arcStartAngle = getPolarAngle( startPoint - centerPoint );
            double arcEndAngle   = getPolarAngle( endPoint - centerPoint );
            double arcAngleDeciDeg = arcEndAngle - arcStartAngle;

            if( cw )
                arcAngleDeciDeg = NormalizeAnglePos( arcAngleDeciDeg );
            else
                arcAngleDeciDeg = NormalizeAngleNeg( arcAngleDeciDeg );

            SHAPE_ARC tempArc( VECTOR2I(centerPoint), VECTOR2I(startPoint), arcAngleDeciDeg / 10.0 );
            SHAPE_LINE_CHAIN arcSegments = tempArc.ConvertToPolyline( Millimeter2iu( 0.1 ) );

            // Load the arc as a series of piece-wise segments

            for( int jj = 0; jj < arcSegments.SegmentCount(); jj++ )
            {
                wxPoint segStart = (wxPoint) arcSegments.Segment( jj ).A;
                wxPoint segEnd   = (wxPoint) arcSegments.Segment( jj ).B;

                loadGraphicStaightSegment( segStart, segEnd, aCadstarLineCodeID,
                        aCadstarSheetID, aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg,
                        aScalingFactor, aTransformCentre, aMirrorInvert );
            }
        }
            break;

        case VERTEX_TYPE::POINT:
            loadGraphicStaightSegment( startPoint, endPoint, aCadstarLineCodeID, aCadstarSheetID,
                    aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg, aScalingFactor,
                    aTransformCentre, aMirrorInvert );
            break;

        default:
            wxFAIL_MSG( "Unknown CADSTAR Vertex type" );
        }


        prev = cur;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadFigure( const FIGURE& aCadstarFigure,
        const LAYER_ID& aCadstarSheetIDOverride, SCH_LAYER_ID aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    loadShapeVertices( aCadstarFigure.Shape.Vertices, aCadstarFigure.LineCodeID,
            aCadstarSheetIDOverride, aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg,
            aScalingFactor, aTransformCentre, aMirrorInvert );

    for( CUTOUT cutout : aCadstarFigure.Shape.Cutouts )
    {
        loadShapeVertices( cutout.Vertices, aCadstarFigure.LineCodeID, aCadstarSheetIDOverride,
                aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg, aScalingFactor,
                aTransformCentre, aMirrorInvert );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheetAndChildSheets(
        LAYER_ID aCadstarSheetID, wxPoint aPosition, wxSize aSheetSize, const SCH_SHEET_PATH& aParentSheet )
{
    wxCHECK_MSG( mSheetMap.find( aCadstarSheetID ) == mSheetMap.end(), , "Sheet already loaded!" );

    SCH_SHEET*  sheet  = new SCH_SHEET( aParentSheet.Last(), aPosition );
    SCH_SCREEN* screen = new SCH_SCREEN( mSchematic );
    SCH_SHEET_PATH instance( aParentSheet );

    sheet->SetSize( aSheetSize );
    sheet->SetScreen( screen );

    wxString name = Sheets.SheetNames.at( aCadstarSheetID );

    SCH_FIELD& sheetNameField = sheet->GetFields()[SHEETNAME];
    SCH_FIELD& filenameField  = sheet->GetFields()[SHEETFILENAME];

    sheetNameField.SetText( name );

    int sheetNum = getSheetNumber( aCadstarSheetID );
    wxString  loadedFilename = wxFileName( Filename ).GetName();
    std::string filename = wxString::Format( "%s_%02d", loadedFilename, sheetNum ).ToStdString();

    ReplaceIllegalFileNameChars( &filename );
    filename += wxT( "." ) + KiCadSchematicFileExtension;

    filenameField.SetText( filename );
    wxFileName fn( filename );
    sheet->GetScreen()->SetFileName( fn.GetFullPath() );
    aParentSheet.Last()->GetScreen()->Append( sheet );
    instance.push_back( sheet );
    sheet->AddInstance( instance.Path() );

    wxString pageNumStr = wxString::Format( "%d", getSheetNumber( aCadstarSheetID ) );
    sheet->SetPageNumber( instance, pageNumStr );

    mSheetMap.insert( { aCadstarSheetID, sheet } );

    loadChildSheets( aCadstarSheetID, instance );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadChildSheets(
        LAYER_ID aCadstarSheetID, const SCH_SHEET_PATH& aSheet )
{
    wxCHECK_MSG( mSheetMap.find( aCadstarSheetID ) != mSheetMap.end(), ,
            "FIXME! Parent sheet should be loaded before attempting to load subsheets" );

    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK& block = blockPair.second;

        if( block.LayerID == aCadstarSheetID && block.Type == BLOCK::TYPE::CHILD )
        {
            if( block.AssocLayerID == wxT( "NO_LINK" ) )
            {
                if( block.Figures.size() > 0 )
                {
                    wxLogError( wxString::Format(
                            _( "The block ID %s (Block name: '%s') is drawn on sheet '%s' but is "
                               "not linked to another sheet in the design. KiCad requires all "
                               "sheet symbols to be associated to a sheet, so the block was not "
                               "loaded." ),
                            block.ID, block.Name, Sheets.SheetNames.at( aCadstarSheetID ) ) );
                }

                continue;
            }

            // In KiCad you can only draw rectangular shapes whereas in Cadstar arbitrary shapes
            // are allowed. We will calculate the extents of the Cadstar shape and draw a rectangle

            std::pair<wxPoint, wxSize> blockExtents;

            if( block.Figures.size() > 0 )
            {
                blockExtents = getFigureExtentsKiCad( block.Figures.begin()->second );
            }
            else
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "The CADSTAR schematic might be corrupt: Block %s references a "
                           "child sheet but has no Figure defined." ),
                        block.ID ) );
            }

            loadSheetAndChildSheets( block.AssocLayerID, blockExtents.first, blockExtents.second, aSheet );

            if( block.HasBlockLabel )
            {
                // Add the block label as a separate field
                SCH_SHEET* loadedSheet = mSheetMap.at( block.AssocLayerID );
                SCH_FIELDS fields      = loadedSheet->GetFields();

                for( SCH_FIELD& field : fields )
                {
                    field.SetVisible( false );
                }

                SCH_FIELD blockNameField( getKiCadPoint( block.BlockLabel.Position ), 2,
                        loadedSheet, wxString( "Block name" ) );
                blockNameField.SetTextAngle( getAngleTenthDegree( block.BlockLabel.OrientAngle ) );
                blockNameField.SetText( block.Name );
                blockNameField.SetVisible( true );
                applyTextSettings( block.BlockLabel.TextCodeID, block.BlockLabel.Alignment,
                        block.BlockLabel.Justification, &blockNameField );
                fields.push_back( blockNameField );
                loadedSheet->SetFields( fields );
            }
        }
    }
}


std::vector<CADSTAR_SCH_ARCHIVE_LOADER::LAYER_ID> CADSTAR_SCH_ARCHIVE_LOADER::findOrphanSheets()
{
    std::vector<LAYER_ID> childSheets, orphanSheets;

    //Find all sheets that are child of another
    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK&    block        = blockPair.second;
        LAYER_ID& assocSheetID = block.AssocLayerID;

        if( block.Type == BLOCK::TYPE::CHILD )
            childSheets.push_back( assocSheetID );
    }

    //Add sheets that do not have a parent
    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( std::find( childSheets.begin(), childSheets.end(), sheetID ) == childSheets.end() )
            orphanSheets.push_back( sheetID );
    }

    return orphanSheets;
}


int CADSTAR_SCH_ARCHIVE_LOADER::getSheetNumber( LAYER_ID aCadstarSheetID )
{
    int i = 1;

    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( sheetID == aCadstarSheetID )
            return i;

        ++i;
    }

    return -1;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadItemOntoKiCadSheet( LAYER_ID aCadstarSheetID, SCH_ITEM* aItem )
{
    wxCHECK_MSG( aItem, /*void*/, "aItem is null" );

    if( aCadstarSheetID == "ALL_SHEETS" )
    {
        SCH_ITEM* duplicateItem;

        for( std::pair<LAYER_ID, SHEET_NAME> sheetPair : Sheets.SheetNames )
        {
            LAYER_ID sheetID = sheetPair.first;
            duplicateItem    = aItem->Duplicate();
            mSheetMap.at( sheetID )->GetScreen()->Append( aItem->Duplicate() );
        }

        //Get rid of the extra copy:
        delete aItem;
        aItem = duplicateItem;
    }
    else if( aCadstarSheetID == "NO_SHEET" )
    {
        wxASSERT_MSG(
                false, "Trying to add an item to NO_SHEET? This might be a documentation symbol." );
    }
    else
    {
        if( mSheetMap.find( aCadstarSheetID ) != mSheetMap.end() )
        {
            mSheetMap.at( aCadstarSheetID )->GetScreen()->Append( aItem );
        }
        else
        {
            delete aItem;
            wxASSERT_MSG( false, "Unknown Sheet ID." );
        }
    }
}


CADSTAR_SCH_ARCHIVE_LOADER::SYMDEF_ID CADSTAR_SCH_ARCHIVE_LOADER::getSymDefFromName(
        const wxString& aSymdefName, const wxString& aSymDefAlternate )
{
    // Do a case-insensitive comparison
    for( std::pair<SYMDEF_ID, SYMDEF_SCM> symPair : Library.SymbolDefinitions )
    {
        SYMDEF_ID  id     = symPair.first;
        SYMDEF_SCM symdef = symPair.second;

        if( symdef.ReferenceName.Lower() == aSymdefName.Lower()
            && symdef.Alternate.Lower() == aSymDefAlternate.Lower() )
        {
            return id;
        }
    }

    return SYMDEF_ID();
}

bool CADSTAR_SCH_ARCHIVE_LOADER::isAttributeVisible( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    // Use CADSTAR visibility settings to determine if an attribute is visible
    if( AttrColors.AttributeColors.find( aCadstarAttributeID ) != AttrColors.AttributeColors.end() )
    {
        return AttrColors.AttributeColors.at( aCadstarAttributeID ).IsVisible;
    }

    return false; // If there is no visibility setting, assume not displayed
}


int CADSTAR_SCH_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            mSchematic->Settings().m_DefaultWireThickness );

    return getKiCadLength( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Width );
}


PLOT_DASH_TYPE CADSTAR_SCH_ARCHIVE_LOADER::getLineStyle( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            PLOT_DASH_TYPE::SOLID );

    // clang-format off
    switch( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Style )
    {
    case LINESTYLE::DASH:       return PLOT_DASH_TYPE::DASH;
    case LINESTYLE::DASHDOT:    return PLOT_DASH_TYPE::DASHDOT;
    case LINESTYLE::DASHDOTDOT: return PLOT_DASH_TYPE::DASHDOT; //TODO: update in future
    case LINESTYLE::DOT:        return PLOT_DASH_TYPE::DOT;
    case LINESTYLE::SOLID:      return PLOT_DASH_TYPE::SOLID;
    default:                    return PLOT_DASH_TYPE::DEFAULT;
    }
    // clang-format on

    return PLOT_DASH_TYPE();
}


CADSTAR_SCH_ARCHIVE_LOADER::TEXTCODE CADSTAR_SCH_ARCHIVE_LOADER::getTextCode(
        const TEXTCODE_ID& aCadstarTextCodeID )
{
    wxCHECK( Assignments.Codedefs.TextCodes.find( aCadstarTextCodeID )
                     != Assignments.Codedefs.TextCodes.end(),
            TEXTCODE() );

    return Assignments.Codedefs.TextCodes.at( aCadstarTextCodeID );
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    wxCHECK( Assignments.Codedefs.AttributeNames.find( aCadstarAttributeID )
                     != Assignments.Codedefs.AttributeNames.end(),
            wxEmptyString );

    return Assignments.Codedefs.AttributeNames.at( aCadstarAttributeID ).Name;
}


CADSTAR_SCH_ARCHIVE_LOADER::PART CADSTAR_SCH_ARCHIVE_LOADER::getPart(
        const PART_ID& aCadstarPartID )
{
    wxCHECK( Parts.PartDefinitions.find( aCadstarPartID ) != Parts.PartDefinitions.end(), PART() );

    return Parts.PartDefinitions.at( aCadstarPartID );
}


CADSTAR_SCH_ARCHIVE_LOADER::ROUTECODE CADSTAR_SCH_ARCHIVE_LOADER::getRouteCode(
        const ROUTECODE_ID& aCadstarRouteCodeID )
{
    wxCHECK( Assignments.Codedefs.RouteCodes.find( aCadstarRouteCodeID )
                     != Assignments.Codedefs.RouteCodes.end(),
            ROUTECODE() );

    return Assignments.Codedefs.RouteCodes.at( aCadstarRouteCodeID );
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getAttributeValue( const ATTRIBUTE_ID& aCadstarAttributeID,
        const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>&                      aCadstarAttributeMap )
{
    wxCHECK( aCadstarAttributeMap.find( aCadstarAttributeID ) != aCadstarAttributeMap.end(),
            wxEmptyString );

    return aCadstarAttributeMap.at( aCadstarAttributeID ).Value;
}


CADSTAR_SCH_ARCHIVE_LOADER::PART::DEFINITION::PIN CADSTAR_SCH_ARCHIVE_LOADER::getPartDefinitionPin(
        const PART& aCadstarPart, const GATE_ID& aGateID, const TERMINAL_ID& aTerminalID )
{
    for( std::pair<PART_DEFINITION_PIN_ID, PART::DEFINITION::PIN> pinPair :
            aCadstarPart.Definition.Pins )
    {
        PART::DEFINITION::PIN partPin = pinPair.second;

        if( partPin.TerminalGate == aGateID && partPin.TerminalPin == aTerminalID )
            return partPin;
    }

    return PART::DEFINITION::PIN();
}


ELECTRICAL_PINTYPE CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPinType( const PART::PIN_TYPE& aPinType )
{
    switch( aPinType )
    {
    case PART::PIN_TYPE::UNCOMMITTED:        return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case PART::PIN_TYPE::INPUT:              return ELECTRICAL_PINTYPE::PT_INPUT;
    case PART::PIN_TYPE::OUTPUT_OR:          return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case PART::PIN_TYPE::OUTPUT_NOT_OR:      return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case PART::PIN_TYPE::OUTPUT_NOT_NORM_OR: return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case PART::PIN_TYPE::POWER:              return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case PART::PIN_TYPE::GROUND:             return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case PART::PIN_TYPE::TRISTATE_BIDIR:     return ELECTRICAL_PINTYPE::PT_BIDI;
    case PART::PIN_TYPE::TRISTATE_INPUT:     return ELECTRICAL_PINTYPE::PT_INPUT;
    case PART::PIN_TYPE::TRISTATE_DRIVER:    return ELECTRICAL_PINTYPE::PT_OUTPUT;
    }

    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}

int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID )
{
    if( aCadstarGateID.IsEmpty() )
        return 1;

    return (int) aCadstarGateID.Upper().GetChar( 0 ) - (int) wxUniChar( 'A' ) + 1;
}


LABEL_SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyle(
        const long long& aCadstarOrientation, bool aMirror )
{
    double           orientationDeciDegree = getAngleTenthDegree( aCadstarOrientation );
    LABEL_SPIN_STYLE spinStyle             = getSpinStyleDeciDeg( orientationDeciDegree );

    if( aMirror )
    {
        spinStyle = spinStyle.RotateCCW();
        spinStyle = spinStyle.RotateCCW();
    }

    return spinStyle;
}


LABEL_SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyleDeciDeg(
        const double& aOrientationDeciDeg )
{
    LABEL_SPIN_STYLE spinStyle = LABEL_SPIN_STYLE::LEFT;

    int oDeg = (int) NormalizeAngle180( aOrientationDeciDeg );

    if( oDeg >= -450 && oDeg <= 450 )
        spinStyle = LABEL_SPIN_STYLE::RIGHT; // 0deg
    else if( oDeg >= 450 && oDeg <= 1350 )
        spinStyle = LABEL_SPIN_STYLE::UP; // 90deg
    else if( oDeg >= 1350 || oDeg <= -1350 )
        spinStyle = LABEL_SPIN_STYLE::LEFT; // 180deg
    else
        spinStyle = LABEL_SPIN_STYLE::BOTTOM; // 270deg

    return spinStyle;
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyTextSettings( const TEXTCODE_ID& aCadstarTextCodeID,
        const ALIGNMENT& aCadstarAlignment, const JUSTIFICATION& aCadstarJustification,
        EDA_TEXT* aKiCadTextItem )
{
    TEXTCODE textCode = getTextCode( aCadstarTextCodeID );
    int      textHeight = KiROUND( (double) getKiCadLength( textCode.Height ) * TXT_HEIGHT_RATIO );
    int      textWidth = getKiCadLength( textCode.Width );

    // The width is zero for all non-cadstar fonts. Using a width equal to the height seems
    // to work well for most fonts.
    if( textWidth == 0 )
        textWidth = getKiCadLength( textCode.Height );

    aKiCadTextItem->SetTextWidth( textWidth );
    aKiCadTextItem->SetTextHeight( textHeight );
    aKiCadTextItem->SetTextThickness( getKiCadLength( textCode.LineWidth ) );

    switch( aCadstarAlignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Bottom left of the first line
        FixTextPositionNoAlignment( aKiCadTextItem );
        KI_FALLTHROUGH;
    case ALIGNMENT::BOTTOMLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;
    }
}

SCH_TEXT* CADSTAR_SCH_ARCHIVE_LOADER::getKiCadSchText( const TEXT& aCadstarTextElement )
{
    SCH_TEXT* kiTxt = new SCH_TEXT();

    kiTxt->SetPosition( getKiCadPoint( aCadstarTextElement.Position ) );
    kiTxt->SetText( aCadstarTextElement.Text );
    kiTxt->SetTextAngle( getAngleTenthDegree( aCadstarTextElement.OrientAngle ) );
    kiTxt->SetMirrored( aCadstarTextElement.Mirror );
    applyTextSettings( aCadstarTextElement.TextCodeID, aCadstarTextElement.Alignment,
            aCadstarTextElement.Justification, kiTxt );

    return kiTxt;
}


std::pair<wxPoint, wxSize> CADSTAR_SCH_ARCHIVE_LOADER::getFigureExtentsKiCad(
        const FIGURE& aCadstarFigure )
{
    wxPoint upperLeft( Assignments.Settings.DesignLimit.x, 0 );
    wxPoint lowerRight( 0, Assignments.Settings.DesignLimit.y );

    for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
    {
        if( upperLeft.x > v.End.x )
            upperLeft.x = v.End.x;

        if( upperLeft.y < v.End.y )
            upperLeft.y = v.End.y;

        if( lowerRight.x < v.End.x )
            lowerRight.x = v.End.x;

        if( lowerRight.y > v.End.y )
            lowerRight.y = v.End.y;
    }

    for( CUTOUT cutout : aCadstarFigure.Shape.Cutouts )
    {
        for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
        {
            if( upperLeft.x > v.End.x )
                upperLeft.x = v.End.x;

            if( upperLeft.y < v.End.y )
                upperLeft.y = v.End.y;

            if( lowerRight.x < v.End.x )
                lowerRight.x = v.End.x;

            if( lowerRight.y > v.End.y )
                lowerRight.y = v.End.y;
        }
    }

    wxPoint upperLeftKiCad  = getKiCadPoint( upperLeft );
    wxPoint lowerRightKiCad = getKiCadPoint( lowerRight );

    wxPoint size = lowerRightKiCad - upperLeftKiCad;

    return { upperLeftKiCad, wxSize( abs( size.x ), abs( size.y ) ) };
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPoint( wxPoint aCadstarPoint )
{
    wxPoint retval;

    retval.x = ( aCadstarPoint.x - mDesignCenter.x ) * KiCadUnitMultiplier;
    retval.y = -( aCadstarPoint.y - mDesignCenter.y ) * KiCadUnitMultiplier;

    return retval;
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::getKiCadLibraryPoint(
        wxPoint aCadstarPoint, wxPoint aCadstarCentre )
{
    wxPoint retval;

    retval.x = ( aCadstarPoint.x - aCadstarCentre.x ) * KiCadUnitMultiplier;
    retval.y = ( aCadstarPoint.y - aCadstarCentre.y ) * KiCadUnitMultiplier;

    return retval;
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::applyTransform( const wxPoint& aPoint,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    wxPoint retVal = aPoint;

    if( aScalingFactor != 1.0 )
    {
        //scale point
        retVal -= aTransformCentre;
        retVal.x = KiROUND( retVal.x * aScalingFactor );
        retVal.y = KiROUND( retVal.y * aScalingFactor );
        retVal += aTransformCentre;
    }

    if( aMirrorInvert )
    {
        MIRROR( retVal.x, aTransformCentre.x );
        MIRROR( retVal.x, aTransformCentre.x );
    }

    if( aRotationAngleDeciDeg != 0.0 )
    {
        RotatePoint( &retVal, aTransformCentre, aRotationAngleDeciDeg );
    }

    if( aMoveVector != wxPoint{ 0, 0 } )
    {
        retVal += aMoveVector;
    }

    return retVal;
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarAngle( wxPoint aPoint )
{
    return NormalizeAnglePos( ArcTangente( aPoint.y, aPoint.x ) );
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarRadius( wxPoint aPoint )
{
    return sqrt(
            ( (double) aPoint.x * (double) aPoint.x ) + ( (double) aPoint.y * (double) aPoint.y ) );
}
