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

#ifndef SCH_IO_ORCAD_H_
#define SCH_IO_ORCAD_H_

#include <optional>

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <wx/arrstr.h>

// OpenOrcadParser headers
#include "Database.hpp"


class SCH_SHEET;
class SCH_SCREEN;


class SCH_IO_ORCAD : public SCH_IO
{
public:
    SCH_IO_ORCAD() : SCH_IO( wxS( "OrCAD Database" ) ), m_db{}
    {
        m_reporter = &WXLOG_REPORTER::GetInstance();
    }

    ~SCH_IO_ORCAD() {}

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "OrCAD Design files" ),
                                      { FILEEXT::OrCadDesignFileExtension, FILEEXT::OrCadDesignBackupFileExtension } );
    }


    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "OrCAD Library files" ),
                                      { FILEEXT::OrCadLibraryFileExtension, FILEEXT::OrCadLibraryBackupFileExtension } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    std::optional<Database> m_db;
};


#endif // SCH_IO_ORCAD_H_
