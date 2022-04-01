/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 Kicad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_CABLE_SIZE_H
#define PANEL_CABLE_SIZE_H

#include "panel_cable_size_base.h"
#include <vector>

class PCB_CALCULATOR_SETTINGS;

class CABLE_SIZE_ENTRY
{
public:
    CABLE_SIZE_ENTRY( wxString aName, double aRadius );
    wxString m_name;
    double   m_radius; // stored in m
};

class PANEL_CABLE_SIZE : public PANEL_CABLE_SIZE_BASE
{
public:
    PANEL_CABLE_SIZE( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_CABLE_SIZE();

    // Methods from CALCULATOR_PANEL that must be overriden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;


    void OnUnitChange( wxCommandEvent& event ) override;
    void OnCurrentChange( wxCommandEvent& event ) override;
    void OnLengthChange( wxCommandEvent& event ) override;
    void OnChangeUnitLen( wxCommandEvent& event ) override;

private:
    bool m_imperial;
    void updateRow( int aRow, double aRadius );
    void updateRows();
    void UpdateColLabels( bool aImperial );

    double m_current;
    double m_length;

    std::vector<CABLE_SIZE_ENTRY> m_entries;

    void onParameterUpdate( wxGridEvent& event );

    /*
    void updateRow( int aRow, bool aInit=false );
    void updateRows();
    void OnUpdateLength();
*/
};

#endif
