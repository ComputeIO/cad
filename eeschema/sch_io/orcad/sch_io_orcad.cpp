/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Dominik Wernberger <dominik.wernberger@gmx.de>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cmath>
#include <filesystem>
#include <memory>

#include "sch_io_orcad.h"
#include "orcad2kicad.h"

#include <kiplatform/environment.h>
#include <project_sch.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_text.h>
#include <schematic.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>

#include <wx/log.h>
#include <wx/stdstream.h>
#include <wx/zipstrm.h>

#include <nlohmann/json.hpp>
#include <core/map_helpers.h>
#include <wx/wfstream.h>

// OpenOrcadParser Headers
#include "Container.hpp"
#include "Enums/DirectoryType.hpp"
#include "GetStreamHelper.hpp"
#include "PinShape.hpp"
#include "Primitives/PrimArc.hpp"
#include "Primitives/PrimBezier.hpp"
#include "Primitives/PrimCommentText.hpp"
#include "Primitives/PrimLine.hpp"
#include "Primitives/PrimPolygon.hpp"
#include "Primitives/PrimPolyline.hpp"
#include "Primitives/PrimRect.hpp"
#include "Streams/StreamPackage.hpp"


namespace fs = std::filesystem;


Database parseDatabase(const wxString& aLibraryPath)
{

    const fs::path dbPath{std::string{aLibraryPath.mb_str()}};
    Container parser{dbPath, ParserConfig{}};

    // ContainerContext& ctx = parser.getContext();

    parser.parseDatabaseFile();

    Database db = parser.getDb();

    return db;
}


void dummy_create_in_schematic(SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet)
{
    std::unique_ptr<LIB_SYMBOL> ksymbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );

    LIB_ID libId;
    libId.Parse( "foo", true );

    ksymbol->SetPower();
    ksymbol->SetLibId( libId );
    ksymbol->SetName( "Name" );
    ksymbol->GetReferenceField().SetText( wxS( "#PWR" ) );
    ksymbol->GetReferenceField().SetVisible( false );
    ksymbol->GetValueField().SetText( "Name" );
    ksymbol->GetValueField().SetVisible( true );
    ksymbol->SetDescription( wxString::Format( _( "Power symbol creates a global "
                                                  "label with name '%s'" ),
                                               "Name" ) );
    ksymbol->SetKeyWords( wxS( "power-flag" ) );
    ksymbol->SetShowPinNames( false );
    ksymbol->SetShowPinNumbers( false );

    std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( ksymbol.get() );

    pin->SetName( "Name" );
    pin->SetNumber( wxS( "1" ) );
    pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    pin->SetLength( 0 );

    ksymbol->AddDrawItem( pin.release() );



    LIB_SYMBOL* pwrLibSym = ksymbol.release();



    std::unique_ptr<SCH_SYMBOL> schSym = std::make_unique<SCH_SYMBOL>(
            *pwrLibSym, libId, &aSchematic->CurrentSheet(), 0 );

    VECTOR2D pos( 0, 0 );
    schSym->SetPosition( pos );
    schSym->SetRef( &aSchematic->CurrentSheet(), "asdf" );

    SCH_SCREEN* screen = aRootSheet->GetScreen();
    screen->Append( schSym.release() );
}


bool SCH_IO_ORCAD::CanReadSchematicFile( const wxString& aFileName ) const
{
    // Not yet supported
    return false;
}


bool SCH_IO_ORCAD::CanReadLibrary( const wxString& aFileName ) const
{
    return true;
}


int SCH_IO_ORCAD::GetModifyHash() const
{
    return 0;
}


void SCH_IO_ORCAD::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                         const wxString&        aLibraryPath,
                                         const STRING_UTF8_MAP* aProperties )
{
    if( !m_db.has_value() )
    {
        m_db = parseDatabase( aLibraryPath );
    }

    const auto dir = getDirectoryStreamFromDb<StreamPackagesDirectory>(
            m_db.value(), DirectoryType::PackagesDirectory );

    if( dir )
    {
        for( const auto& item : dir->items )
        {
            std::cout << "Enumerate package " << item.name << std::endl;
            aSymbolNameList.Add( wxString{item.name.c_str(), wxConvUTF8} );
        }
    }

    std::cout << "Enumerated " << aSymbolNameList.size() << " packages" << std::endl;
}


void SCH_IO_ORCAD::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                         const wxString&           aLibraryPath,
                                         const STRING_UTF8_MAP*    aProperties )
{
    wxArrayString aSymbolNameList{};
    EnumerateSymbolLib( aSymbolNameList, aLibraryPath, aProperties );

    for(const auto& name : aSymbolNameList )
    {
        std::cout << "Loading package " << name << std::endl;
        const auto symbol = LoadSymbol( aLibraryPath, name, aProperties );
        aSymbolList.push_back( symbol );
    }

    std::cout << "Loaded " << aSymbolNameList.size() << " packages" << std::endl;
}


LIB_SYMBOL* SCH_IO_ORCAD::LoadSymbol( const wxString&        aLibraryPath,
                                        const wxString&        aAliasName,
                                        const STRING_UTF8_MAP* aProperties )
{
    std::cout << "Loading " << aAliasName << std::endl;

    if( !m_db.has_value() )
    {
        m_db = parseDatabase( aLibraryPath );
    }

    std::shared_ptr<StreamPackage> package{};

    for( auto& stream : m_db.value().mStreams )
    {
        if( stream->mCtx.mCfbfStreamLocation.matches_pattern({{"Packages"}, {std::string{aAliasName.mb_str()}}}) )
        {
            package = std::dynamic_pointer_cast<StreamPackage>( stream );
            break;
        }
    }

    LIB_SYMBOL* ksymbol = new LIB_SYMBOL( wxEmptyString );

    if(package)
    {
        wxString name{"Dummy", wxConvUTF8};
        wxString refDes{"X", wxConvUTF8};
        wxString pcbFootprint{"", wxConvUTF8};

        if( package->package )
        {
            name = wxString{package->package->name.c_str(), wxConvUTF8};
            refDes = wxString{package->package->refDes.c_str(), wxConvUTF8};
            pcbFootprint = wxString{package->package->pcbFootprint.c_str(), wxConvUTF8};
        }

        for( const auto& libraryPart : package->libraryParts )
        {
            if( !libraryPart )
            {
                continue;
            }

            for( const auto& primitive : libraryPart->primitives )
            {
                switch( primitive->getObjectType() )
                {
                    case Primitive::Arc:
                    {
                        const auto orcadObj = dynamic_cast<const PrimArc*>( primitive.get() );
                        auto kicadObj = convPrimArc2ARC( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Bezier:
                    {
                        const auto orcadObj = dynamic_cast<const PrimBezier*>( primitive.get() );
                        auto segments = convPrimBezier2BEZIER( orcadObj );

                        for( auto& segment : segments )
                        {
                            ksymbol->AddDrawItem( segment );
                        }

                        break;
                    }
                    case Primitive::CommentText:
                    {
                        const auto orcadObj = dynamic_cast<const PrimCommentText*>( primitive.get() );
                        auto kicadObj = convPrimCommentText2SCH_TEXT( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Ellipse:
                    {
                        const auto orcadObj = dynamic_cast<const PrimEllipse*>( primitive.get() );
                        auto kicadObj = convPrimEllipse2POLY( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Line:
                    {
                        const auto orcadObj = dynamic_cast<const PrimLine*>( primitive.get() );
                        auto kicadObj = convPrimLine2POLY( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Polygon:
                    {
                        const auto orcadObj = dynamic_cast<const PrimPolygon*>( primitive.get() );
                        auto kicadObj = convPrimPolygon2POLY( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Polyline:
                    {
                        const auto orcadObj = dynamic_cast<const PrimPolyline*>( primitive.get() );
                        auto kicadObj = convPrimPolyline2POLY( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    case Primitive::Rect:
                    {
                        const auto orcadObj = dynamic_cast<const PrimRect*>( primitive.get() );
                        auto kicadObj = convPrimRect2RECTANGLE( orcadObj );
                        ksymbol->AddDrawItem( kicadObj );
                        break;
                    }
                    default:
                        std::cout << "Unimplemented Primitive detected" << std::endl;
                        break;
                }
            }

            for( size_t i = 0U; i < libraryPart->symbolPins.size(); ++i )
            {
                auto pin = StreamPackage2SCH_PIN( package.get(), ksymbol, i );

                if( pin )
                {
                    ksymbol->AddDrawItem( pin );
                }
            }
        }

        LIB_ID libId{ wxS( "Library" ), name };

        ksymbol->SetLibId( libId );
        ksymbol->SetName( name );

        ksymbol->GetReferenceField().SetText( refDes );
        ksymbol->GetReferenceField().SetVisible( true );

        ksymbol->GetFootprintField().SetText( pcbFootprint );
        ksymbol->GetFootprintField().SetVisible( false );

        ksymbol->GetValueField().SetText( wxS( "" ) );
        ksymbol->GetValueField().SetVisible( true );

        bool pinNameVisible = true;
        bool pinNumberVisible = true;

        if( package && !package->libraryParts.empty() )
        {
            const auto& generalProperties = package->libraryParts.at(0)->generalProperties;

            pinNameVisible = generalProperties.pinNameVisible;
            pinNumberVisible = generalProperties.pinNumberVisible;
        }

        ksymbol->SetShowPinNames( pinNameVisible );
        ksymbol->SetShowPinNumbers( pinNumberVisible );
    }

    return ksymbol;
}


SCH_SHEET* SCH_IO_ORCAD::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                              SCH_SHEET*             aAppendToMe,
                                              const STRING_UTF8_MAP* aProperties )
{
    std::cout << "Loading schematic " << std::endl;

    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxS( "Can't append to a schematic with no root!" ) );

        rootSheet = &aSchematic->Root();
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( aFileName );
        aSchematic->SetRoot( rootSheet );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );

        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );
    }

    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &aSchematic->Prj() );

    wxCHECK_MSG( libTable, nullptr, wxS( "Could not load symbol lib table." ) );

    // @todo
    // LoadSchematic( aSchematic, rootSheet, aFileName );
    dummy_create_in_schematic(aSchematic, rootSheet);

    aSchematic->CurrentSheet().UpdateAllScreenReferences();

    return rootSheet;
}
