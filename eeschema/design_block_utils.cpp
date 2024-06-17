/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <design_block_lib_table.h>
#include <sch_edit_frame.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wildcards_and_files_ext.h>
#include <paths.h>
#include <common.h>
#include <kidialog.h>
#include <confirm.h>

DESIGN_BLOCK_LIB_TABLE* SCH_EDIT_FRAME::selectDesignBlockLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( Prj().IsNullProject() )
    {
        DESIGN_BLOCK_LIB_TABLE* ret = &GDesignBlockTable;

        if( aOptional )
        {
            wxMessageDialog dlg( this, _( "Add the library to the global library table?" ),
                                 _( "Add To Global Library Table" ), wxYES_NO );

            if( dlg.ShowModal() != wxID_OK )
                ret = nullptr;
        }

        return ret;
    }

    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( this, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0: return &GDesignBlockTable;
    case 1: return Prj().DesignBlockLibs();
    default: return nullptr;
    }
}


wxString SCH_EDIT_FRAME::CreateNewProjectLibrary( const wxString& aLibName,
                                                  const wxString& aProposedName )
{
    return createNewDesignBlockLibrary( aLibName, aProposedName, Prj().DesignBlockLibs() );
}


wxString SCH_EDIT_FRAME::CreateNewLibrary( const wxString& aLibName, const wxString& aProposedName )
{
    return createNewDesignBlockLibrary( aLibName, aProposedName, selectDesignBlockLibTable() );
}


wxString SCH_EDIT_FRAME::createNewDesignBlockLibrary( const wxString&         aLibName,
                                                      const wxString&         aProposedName,
                                                      DESIGN_BLOCK_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        return wxEmptyString;

    wxString   initialPath = aProposedName.IsEmpty() ? Prj().GetProjectPath() : aProposedName;
    wxFileName fn;
    bool       doAdd = false;
    bool       isGlobal = ( aTable == &GDesignBlockTable );

    if( aLibName.IsEmpty() )
    {
        fn = initialPath;

        if( !LibraryFileBrowser( false, fn, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                 FILEEXT::KiCadDesignBlockLibPathExtension, false, isGlobal,
                                 PATHS::GetDefaultUserFootprintsPath() ) )
        {
            return wxEmptyString;
        }

        doAdd = true;
    }
    else
    {
        fn = EnsureFileExtension( aLibName, FILEEXT::KiCadDesignBlockLibPathExtension );

        if( !fn.IsAbsolute() )
        {
            fn.SetName( aLibName );
            fn.MakeAbsolute( initialPath );
        }
    }

    // We can save db libs only using PCB_IO_MGR::KICAD_SEXP format (.pretty libraries)
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T piType = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;
    wxString                                 libPath = fn.GetFullPath();

    try
    {
        IO_RELEASER<DESIGN_BLOCK_IO> pi( DESIGN_BLOCK_IO_MGR::FindPlugin( piType ) );

        bool writable = false;
        bool exists   = false;

        try
        {
            writable = pi->IsLibraryWritable( libPath );
            exists   = true;    // no exception was thrown, lib must exist.
        }
        catch( const IO_ERROR& )
        {
            // best efforts....
        }

        if( exists )
        {
            if( !writable )
            {
                wxString msg = wxString::Format( _( "Library %s is read only." ), libPath );
                ShowInfoBarError( msg );
                return wxEmptyString;
            }
            else
            {
                wxString msg = wxString::Format( _( "Library %s already exists." ), libPath );
                KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                dlg.SetOKLabel( _( "Overwrite" ) );
                dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

                if( dlg.ShowModal() == wxID_CANCEL )
                    return wxEmptyString;

                pi->DeleteLibrary( libPath );
            }
        }

        pi->CreateLibrary( libPath );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return wxEmptyString;
    }

    //if( doAdd )
        //AddLibrary( libPath, aTable );

    return libPath;
}
