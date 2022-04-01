/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <calculator_panels/panel_cable_size.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <widgets/unit_selector.h>

#define M_to_MM 1000
#define M_to_IN 39.37008
#define M_to_1000FT 0.00328084
#define M2_to_MM2 1000000
#define M_to_KM 0.001

#define COPPER_RESISTIVITY 1.72e-8 // ohm meter
#define VACCUM_PERMEABILITY 1.256637e-6
#define RELATIVE_PERMEABILITY 1

// Define the column ordering
#define col_diameter 0
#define col_area 1
#define col_lin_res 2
#define col_freq 3
#define col_ampacity 4
#define col_res 5
#define col_Vdrop 6
#define col_power 7

CABLE_SIZE_ENTRY::CABLE_SIZE_ENTRY( wxString aName, double aRadius )
{
    m_name = aName;
    m_radius = aRadius;
}

void PANEL_CABLE_SIZE::UpdateColLabels( bool aImperial )
{
    if( aImperial )
    {
        m_table->SetColLabelValue( col_diameter, _( "Diameter\n(inches)" ) );
        m_table->SetColLabelValue( col_lin_res, _( "Linear Resistance\n(Ohm/1000ft)" ) );
    }
    else
    {
        m_table->SetColLabelValue( col_diameter, _( "Diameter\n(mm)" ) );
        m_table->SetColLabelValue( col_lin_res, _( "Linear Resistance\n(Ohm/km)" ) );
    }
    m_table->SetColLabelValue( col_freq, _( "Frequency for 100% skin depth\n(Hz)" ) );
    m_table->SetColLabelValue( col_area, _( "Area\n(mm^2)" ) );
    m_table->SetColLabelValue( col_ampacity, _( "Ampacity\n(A)" ) );
    m_table->SetColLabelValue( col_res, _( "Resistance\n(ohm)" ) );
    m_table->SetColLabelValue( col_Vdrop, _( "Voltage Drop\n(V)" ) );
    m_table->SetColLabelValue( col_power, _( "Power\n(W)" ) );
}


PANEL_CABLE_SIZE::PANEL_CABLE_SIZE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style, const wxString& name ) :
        PANEL_CABLE_SIZE_BASE( parent, id, pos, size, style, name )
{
    m_entries.clear();
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG0000" ), 0.005842 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG000" ), 0.00520192 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG00" ), 0.00463296 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG0" ), 0.00412623 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG1" ), 0.00367411 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG2" ), 0.00327152 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG3" ), 0.00291338 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG4" ), 0.00259461 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG5" ), 0.00231013 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG6" ), 0.00205740 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG7" ), 0.00183261 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG8" ), 0.00163195 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG9" ), 0.00145288 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG10" ), 0.00129413 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG11" ), 0.00115189 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG12" ), 0.00102616 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG13" ), 0.0009144 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG14" ), 0.00081407 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG15" ), 0.00072517 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG16" ), 0.00064516 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG17" ), 0.00057531 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG18" ), 0.00051181 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG19" ), 0.00045593 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG20" ), 0.0004046 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG21" ), 0.00036195 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG22" ), 0.00032258 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG23" ), 0.00028702 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG24" ), 0.00025527 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG25" ), 0.00022773 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG26" ), 0.00020193 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG27" ), 0.00018034 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG28" ), 0.00016002 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG29" ), 0.00014351 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG30" ), 0.000127 ) );

    m_table->DeleteCols( 0, m_table->GetNumberCols() );
    m_table->DeleteRows( 0, m_table->GetNumberRows() );

    m_table->AppendCols( 8 );
    m_table->AppendRows( m_entries.size() + 1 );

    UpdateColLabels( m_imperial );

    int i = 1;
    for( CABLE_SIZE_ENTRY& entry : m_entries )
    {
        m_table->SetRowLabelValue( i, entry.m_name );
        updateRow( i, entry.m_radius );
        m_table->SetReadOnly( i, col_diameter );
        m_table->SetReadOnly( i, col_area );
        m_table->SetReadOnly( i, col_lin_res );
        m_table->SetReadOnly( i, col_ampacity );
        m_table->SetReadOnly( i, col_freq );
        m_table->SetReadOnly( i, col_res );
        m_table->SetReadOnly( i, col_Vdrop );
        m_table->SetReadOnly( i, col_power );
        i++;
    }

    m_table->SetRowLabelValue( 0, wxString( _( "Custom" ) ) );
    m_table->SetReadOnly( 0, col_diameter, false );
    m_table->SetReadOnly( 0, col_area, false );
    m_table->SetReadOnly( 0, col_lin_res, false );
    m_table->SetReadOnly( 0, col_ampacity, false );
    m_table->SetReadOnly( 0, col_freq, false );
    m_table->SetReadOnly( 0, col_res, false );
    m_table->SetReadOnly( 0, col_Vdrop, false );
    m_table->SetReadOnly( 0, col_power, false );

    m_table->Connect( wxEVT_GRID_CELL_CHANGE,
                      wxGridEventHandler( PANEL_CABLE_SIZE::onParameterUpdate ), NULL, this );
    m_table->AutoSize();
}

void PANEL_CABLE_SIZE::onParameterUpdate( wxGridEvent& aEvent )
{
    double   value;
    int      row = aEvent.GetRow();
    int      col = aEvent.GetCol();
    wxString str = m_table->GetCellValue( 0, aEvent.GetCol() );
    bool     status = str.ToDouble( &value );
    status &= ( row == 0 );
    status &= ( value > 0 );

    if( status )
    {
        switch( col )
        {
        case col_diameter: updateRow( 0, value / 2 / ( m_imperial ? M_to_IN : M_to_MM ) ); break;
        case col_area: updateRow( 0, sqrt( value / M_PI ) / M_to_MM ); break;
        case col_lin_res:
            updateRow( 0, sqrt( COPPER_RESISTIVITY / value / M_PI )
                                  / ( m_imperial ? M_to_1000FT : M_to_KM ) );
            break;
        case col_freq:
            updateRow( 0, sqrt( COPPER_RESISTIVITY / value / M_PI / VACCUM_PERMEABILITY
                                / RELATIVE_PERMEABILITY ) );
            break;
        // Based on the 700 circular mils per amp rule of the thumb
        // The long number is the sq m to circular mils conversion
        case col_ampacity: updateRow( 0, sqrt( value * 700 / 1973525241.77 / M_PI ) ); break;
        case col_res: updateRow( 0, sqrt( COPPER_RESISTIVITY / value * m_length / M_PI ) ); break;
        case col_Vdrop:
            updateRow( 0, sqrt( COPPER_RESISTIVITY / value * m_length * m_current / M_PI ) );
            break;
        case col_power:
            updateRow( 0, sqrt( COPPER_RESISTIVITY / value * m_length * m_current * m_current
                                / M_PI ) );
            break;
        }
    }
    else
    {
        m_table->SetCellValue( 0, col_diameter, _( "Error" ) );
        m_table->SetCellValue( 0, col_area, _( "Error" ) );
        m_table->SetCellValue( 0, col_ampacity, _( "Error" ) );
        m_table->SetCellValue( 0, col_freq, _( "Error" ) );
        m_table->SetCellValue( 0, col_lin_res, _( "Error" ) );
        m_table->SetCellValue( 0, col_res, _( "Error" ) );
        m_table->SetCellValue( 0, col_Vdrop, _( "Error" ) );
        m_table->SetCellValue( 0, col_power, _( "Error" ) );
        m_table->SetCellValue( 0, col, str );
    }
}

void PANEL_CABLE_SIZE::OnChangeUnitLen( wxCommandEvent& event )
{
    OnLengthChange( event );
}

void PANEL_CABLE_SIZE::OnUnitChange( wxCommandEvent& event )
{
    bool new_imperial = ( m_radioUnits->GetSelection() == 1 );

    if( m_imperial == new_imperial )
    {
        return;
    }

    m_imperial = new_imperial;
    UpdateColLabels( m_imperial );

    double diameter;
    bool   status = m_table->GetCellValue( 0, col_diameter ).ToDouble( &diameter );

    if( m_imperial )
    {
        diameter = diameter / M_to_MM * M_to_IN;
    }
    else
    {
        diameter = diameter * M_to_MM / M_to_IN;
    }

    m_table->SetCellValue( 0, col_diameter, wxString( "" ) << diameter );
    updateRows();
}

PANEL_CABLE_SIZE::~PANEL_CABLE_SIZE()
{
}


void PANEL_CABLE_SIZE::ThemeChanged()
{
}


void PANEL_CABLE_SIZE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_cableSize.current = m_current;
    aCfg->m_cableSize.length = m_length;
    aCfg->m_cableSize.imperial = m_imperial;
    aCfg->m_cableSize.length_unit = m_choiceUnit->GetSelection();
}


void PANEL_CABLE_SIZE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_current = aCfg->m_cableSize.current;
    m_length = aCfg->m_cableSize.length;
    m_imperial = aCfg->m_cableSize.imperial;
    m_choiceUnit->SetSelection( aCfg->m_cableSize.length_unit );

    m_currentCtrl->SetValue( wxString( "" ) << m_current );
    m_lengthCtrl->SetValue( wxString( "" ) << m_length / m_choiceUnit->GetUnitScale() );
    m_radioUnits->SetSelection( m_imperial ? 1 : 0 );
}

void PANEL_CABLE_SIZE::OnCurrentChange( wxCommandEvent& event )
{
    double value;
    bool   status = m_currentCtrl->GetValue().ToDouble( &value );
    status &= value > 0;

    m_current = status ? value : std::nan( "0" );
    updateRows();
}

void PANEL_CABLE_SIZE::OnLengthChange( wxCommandEvent& event )
{
    double value;
    bool   status = m_lengthCtrl->GetValue().ToDouble( &value );
    status &= value > 0;

    m_length = status ? value * m_choiceUnit->GetUnitScale() : std::nan( "0" );
    updateRows();
}

void PANEL_CABLE_SIZE::updateRows()
{
    int    i = 1;
    double value;
    bool   status = ( m_table->GetCellValue( 0, col_diameter ).ToDouble( &value ) );
    value /= ( m_imperial ? M_to_IN : M_to_MM );

    if( status )
    {
        updateRow( 0, value / 2 );
    }

    for( CABLE_SIZE_ENTRY& entry : m_entries )
    {
        updateRow( i, entry.m_radius );
        i++;
    }
}

void PANEL_CABLE_SIZE::updateRow( int aRow, double aRadius )
{
    double area;
    double linearResistance;
    double maxFrequency;
    double resistance;
    double voltageDrop;
    double dissipatedPower;

    // Update wire properties
    area = M_PI * aRadius * aRadius;
    linearResistance = COPPER_RESISTIVITY / area;
    // max frequency is when skin depth = radius
    maxFrequency = COPPER_RESISTIVITY
                   / ( M_PI * aRadius * aRadius * VACCUM_PERMEABILITY * RELATIVE_PERMEABILITY );

    // Based on the 700 circular mils per amp rule of the thumb
    // The long number is the sq m to circular mils conversion
    double ampacity = ( area * 1973525241.77 ) / 700;
    // Update application-specific values
    resistance = linearResistance * m_length;
    voltageDrop = resistance * m_current;
    dissipatedPower = voltageDrop * m_current;


    wxString value = wxString( "" ) << aRadius * 2 * ( m_imperial ? M_to_IN : M_to_MM );
    m_table->SetCellValue( aRow, col_diameter, value );
    value = wxString( "" ) << area * M2_to_MM2;
    m_table->SetCellValue( aRow, col_area, value );
    value = wxString( "" ) << ampacity;
    m_table->SetCellValue( aRow, col_ampacity, value );
    value = wxString( "" ) << maxFrequency;
    m_table->SetCellValue( aRow, col_freq, value );
    value = wxString( "" ) << linearResistance / ( m_imperial ? M_to_1000FT : M_to_KM );
    m_table->SetCellValue( aRow, col_lin_res, value );

    if( m_length > 0 )
    {
        value = wxString( "" ) << resistance;
        m_table->SetCellValue( aRow, col_res, value );
        value = wxString( "" ) << voltageDrop;
        m_table->SetCellValue( aRow, col_Vdrop, value );
        value = wxString( "" ) << dissipatedPower;
        m_table->SetCellValue( aRow, col_power, value );
    }
    else
    {
        value = "";
        m_table->SetCellValue( aRow, col_res, value );
        m_table->SetCellValue( aRow, col_Vdrop, value );
        m_table->SetCellValue( aRow, col_power, value );
    }
}