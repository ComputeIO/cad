/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SCHEMATIC_H
#define KICAD_SCHEMATIC_H

#include <base_struct.h>
#include <sch_sheet_path.h>
#include <schematic_settings.h>


class BUS_ALIAS;
class CONNECTION_GRAPH;
class EDA_BASE_FRAME;
class ERC_SETTINGS;
class PROJECT;
class SCH_SCREEN;
class SCH_SHEET;

/**
 * Holds all the data relating to one schematic
 * A schematic may consist of one or more sheets (and one root sheet)
 * Right now, eeschema can have only one schematic open at a time, but this could change.
 * Please keep this possibility in mind when adding to this object.
 */
class SCHEMATIC : public EDA_ITEM
{
    friend class SCH_EDIT_FRAME;

private:
    PROJECT* m_project;

    /// The top-level sheet in this schematic hierarchy (or potentially the only one)
    SCH_SHEET* m_rootSheet;

    /**
     * The sheet path of the sheet currently being edited or displayed.
     * Note that this was moved here from SCH_EDIT_FRAME because currently many places in the code
     * want to know the current sheet.  Potentially this can be moved back to the UI code once
     * the only places that want to know it are UI-related
     */
    SCH_SHEET_PATH* m_currentSheet;

    /// Holds and calculates connectivity information of this schematic
    CONNECTION_GRAPH* m_connectionGraph;

public:
    SCHEMATIC( PROJECT* aPrj );

    ~SCHEMATIC();

    virtual wxString GetClass() const override
    {
        return wxT( "SCHEMATIC" );
    }

    /// Initializes this schematic to a blank one, unloading anything existing
    void Reset();

    /// Return a reference to the project this schematic is part of
    PROJECT& Prj() const
    {
        return *const_cast<PROJECT*>( m_project );
    }

    void SetProject( PROJECT* aPrj );

    /// Sets up the template fieldnames link if this project is opened in eeschema
    void SetTemplateFieldNames( TEMPLATES* aTemplates );

    /**
     * Builds and returns an updated schematic hierarchy
     * TODO: can this be cached?
     * @return a SCH_SHEET_LIST containing the schematic hierarchy
     */
    SCH_SHEET_LIST GetSheets() const
    {
        return SCH_SHEET_LIST( m_rootSheet );
    }

    SCH_SHEET& Root() const
    {
        return *m_rootSheet;
    }

    /**
     * Initializes the schematic with a new root sheet.
     * This is typically done by calling a file loader that returns the new root sheet
     * As a side-effect, takes care of some post-load initialization.
     * @param aRootSheet is the new root sheet for this schematic.
     */
    void SetRoot( SCH_SHEET* aRootSheet );

    /// A simple test if the schematic is loaded, not a complete one
    bool IsValid() const
    {
        return m_rootSheet != nullptr;
    }

    /// Helper to retreive the screen of the root sheet
    SCH_SCREEN* RootScreen() const;

    /// Helper to retrieve the filename from the root sheet screen
    wxString GetFileName() const;

    SCH_SHEET_PATH& CurrentSheet() const
    {
        return *m_currentSheet;
    }

    void SetCurrentSheet( const SCH_SHEET_PATH& aPath )
    {
        *m_currentSheet = aPath;
    }

    CONNECTION_GRAPH* ConnectionGraph() const
    {
        return m_connectionGraph;
    }

    SCHEMATIC_SETTINGS& Settings() const;

    ERC_SETTINGS& ErcSettings() const;

    /**
     * Returns a pointer to a bus alias object for the given label,
     * or null if one doesn't exist
     */
    std::shared_ptr<BUS_ALIAS> GetBusAlias( const wxString& aLabel ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif
};

#endif
