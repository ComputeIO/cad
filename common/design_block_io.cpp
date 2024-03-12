/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <i18n_utility.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/translation.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wildcards_and_files_ext.h>
#include <kiway_player.h>
#include <design_block_io.h>
#include <design_block.h>

const wxString DESIGN_BLOCK_IO_MGR::ShowType( DESIGN_BLOCK_FILE_T aFileType )
{
    switch( aFileType )
    {
    case KICAD_SEXP: return _( "KiCad" );
    default: return wxString::Format( _( "UNKNOWN (%d)" ), aFileType );
    }
}


DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T
DESIGN_BLOCK_IO_MGR::EnumFromStr( const wxString& aFileType )
{
    if( aFileType == _( "KiCad" ) )
        return DESIGN_BLOCK_FILE_T( KICAD_SEXP );

    return DESIGN_BLOCK_FILE_T( DESIGN_BLOCK_FILE_UNKNOWN );
}


DESIGN_BLOCK_IO* DESIGN_BLOCK_IO_MGR::FindPlugin( DESIGN_BLOCK_FILE_T aFileType )
{
    switch( aFileType )
    {
    case KICAD_SEXP:          return new DESIGN_BLOCK_IO();
    default:                  return nullptr;
    }
}


DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T
DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl )
{
    if( IO_RELEASER<DESIGN_BLOCK_IO>( FindPlugin( KICAD_SEXP ) )->CanReadLibrary( aLibPath ) && aCtl != KICTL_NONKICAD_ONLY )
        return KICAD_SEXP;

    return DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE;
}


bool DESIGN_BLOCK_IO_MGR::ConvertLibrary( STRING_UTF8_MAP* aOldFileProps,
                                          const wxString&  aOldFilePath,
                                          const wxString&  aNewFilePath )
{
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T oldFileType =
            DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( aOldFilePath );

    if( oldFileType == DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE )
        return false;


    IO_RELEASER<DESIGN_BLOCK_IO> oldFilePI( DESIGN_BLOCK_IO_MGR::FindPlugin( oldFileType ) );
    IO_RELEASER<DESIGN_BLOCK_IO> kicadPI(
            DESIGN_BLOCK_IO_MGR::FindPlugin( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) );
    wxArrayString dbNames;
    wxFileName    newFileName( aNewFilePath );

    if( newFileName.HasExt() )
    {
        wxString extraDir = newFileName.GetFullName();
        newFileName.ClearExt();
        newFileName.SetName( "" );
        newFileName.AppendDir( extraDir );
    }

    if( !newFileName.DirExists() && !wxFileName::Mkdir( aNewFilePath, wxS_DIR_DEFAULT ) )
        return false;

    try
    {
        bool bestEfforts = false; // throw on first error
        oldFilePI->DesignBlockEnumerate( dbNames, aOldFilePath, bestEfforts, aOldFileProps );

        for( const wxString& dbName : dbNames )
        {
            std::unique_ptr<const DESIGN_BLOCK> db(
                    oldFilePI->GetEnumeratedDesignBlock( aOldFilePath, dbName, aOldFileProps ) );
            kicadPI->DesignBlockSave( aNewFilePath, db.get() );
        }
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


const DESIGN_BLOCK_IO::IO_FILE_DESC DESIGN_BLOCK_IO::GetLibraryDesc() const
{
    return IO_BASE::IO_FILE_DESC( _HKI( "KiCad Design Block folders" ), {},
                                  { FILEEXT::KiCadDesignBlockLibPathExtension }, false );
}


long long DESIGN_BLOCK_IO::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void DESIGN_BLOCK_IO::DesignBlockEnumerate( wxArrayString&  aDesignBlockNames,
                                            const wxString& aLibraryPath, bool aBestEfforts,
                                            const STRING_UTF8_MAP* aProperties )
{
    // From the starting directory, look for all directories ending in the .block extension
    wxDir dir( aLibraryPath );

    if( !dir.IsOpened() )
        return;

    wxString dirname;
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::KiCadDesignBlockPathExtension );
    bool     cont = dir.GetFirst( &dirname, fileSpec, wxDIR_DIRS );

    while( cont )
    {
        aDesignBlockNames.Add( dirname.Before( wxT( '.' ) ) );
        cont = dir.GetNext( &dirname );
    }
}


DESIGN_BLOCK* DESIGN_BLOCK_IO::DesignBlockLoad( const wxString& aLibraryPath,
                                                const wxString& aDesignBlockName, bool aKeepUUID,
                                                const STRING_UTF8_MAP* aProperties )
{
    DESIGN_BLOCK* newDB = new DESIGN_BLOCK();
    // Library name needs to be empty for when we will it in with the correct library nickname
    // one layer above
    newDB->SetLibId( LIB_ID( wxEmptyString, aDesignBlockName ) );
    newDB->SetSchematicFile(
            // Library path
            aLibraryPath + wxFileName::GetPathSeparator() +
            // Design block name (project folder)
            aDesignBlockName + +wxT( "." ) + FILEEXT::KiCadDesignBlockPathExtension + wxT( "/" ) +
            // Schematic file
            aDesignBlockName + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension );

    return newDB;
}
