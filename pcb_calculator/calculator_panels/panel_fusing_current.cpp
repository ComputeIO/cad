/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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

// See equation 9b in this paper:
// https://adam-research.de/pdfs/TRM_WhitePaper10_AdiabaticWire.pdf

// Onderkonk equation uses circular mils, but this constant uses mm^2
// 33 * 0.0005067075 ^2
#define ONDERKONK_COEFF 8.472e-6

#include <calculator_panels/panel_fusing_current.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>

#include <widgets/unit_selector.h>

#include <i18n_utility.h> // For _HKI definition
wxString fusing_current_help =
#include "fusing_current_help.h"


PANEL_FUSING_CURRENT::PANEL_FUSING_CURRENT( wxWindow * parent, wxWindowID id,
                                            const wxPoint& pos, const wxSize& size,
                                            long style, const wxString& name ) :
PANEL_FUSING_CURRENT_BASE( parent, id, pos, size, style, name )
{
    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( fusing_current_help, msg );
    m_htmlHelp->SetPage( msg );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_FUSING_CURRENT::~PANEL_FUSING_CURRENT()
{
}


void PANEL_FUSING_CURRENT::ThemeChanged()
{
    // Update the HTML window with the help text
    m_htmlHelp->ThemeChanged();
}


void PANEL_FUSING_CURRENT::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_FUSING_CURRENT::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}

void PANEL_FUSING_CURRENT::m_onCalculateClick( wxCommandEvent& event )
{
    double Tm, Ta, I, W, T, time;
    bool   valid_Tm, valid_Ta, valid_I, valid_W, valid_T, valid_time;

    valid_Tm   = m_meltingValue->GetValue().ToDouble( &Tm );
    valid_Ta   = m_ambientValue->GetValue().ToDouble( &Ta );
    valid_I    = m_currentValue->GetValue().ToDouble( &I );
    valid_W    = m_widthValue->GetValue().ToDouble( &W );
    valid_T    = m_thicknessValue->GetValue().ToDouble( &T );
    valid_time = m_timeValue->GetValue().ToDouble( &time );

    double scalingT, scalingW;

    scalingT = m_thicknessUnit->GetUnitScale() * 1000;
    scalingW = m_widthUnit->GetUnitScale() * 1000;
    T *= scalingT;
    W *= scalingW;

    valid_Tm   &= std::isfinite( Tm );
    valid_Ta   &= std::isfinite( Ta );
    valid_I    &= std::isfinite( I );
    valid_W    &= std::isfinite( W );
    valid_T    &= std::isfinite( T );
    valid_time &= std::isfinite( time );

    if( valid_Tm && valid_Ta )
    {
        valid_Tm &= Tm > Ta;
        valid_Ta &= Tm > Ta;
    }

    valid_I    &= ( I > 0 );
    valid_W    &= ( W > 0 );
    valid_T    &= ( T > 0 );
    valid_time &= ( time > 0 );

    double A     = W * T;
    double ftemp = 1; // Temperature function

    if( valid_Ta && valid_Tm ) // Avoid division by 0 and log of negative
        ftemp = log10( ( Tm - Ta ) / ( 233 + Ta ) + 1 );

    if( m_widthRadio->GetValue() )
    {
        if( valid_I && valid_T && valid_Ta && valid_Tm && valid_Tm && valid_time )
        {
            A = I / sqrt( ftemp / time / ONDERKONK_COEFF );
            W = A / T;
            m_widthValue->SetValue( wxString::Format( wxT( "%f" ), W / scalingW ) );
        }
        else
        {
            m_widthValue->SetValue( "Error" );
        }
    }
    else if( m_thicknessRadio->GetValue() )
    {
        if( valid_I && valid_W && valid_Ta && valid_Tm && valid_Tm && valid_time )
        {
            A = I / sqrt( ftemp / time / ONDERKONK_COEFF );
            T = A / W;
            m_thicknessValue->SetValue( wxString::Format( wxT( "%f" ), T / scalingT ) );
        }
        else
        {
            m_thicknessValue->SetValue( "Error" );
        }
    }
    else if( m_currentRadio->GetValue() )
    {
        if( valid_W && valid_T && valid_Ta && valid_Tm && valid_Tm && valid_time )
        {
            I = A * sqrt( ftemp / time / ONDERKONK_COEFF );
            m_currentValue->SetValue( wxString::Format( wxT( "%f" ), I ) );
        }
        else
        {
            m_currentValue->SetValue( "Error" );
        }
    }
    else if( m_timeRadio->GetValue() )
    {
        if( valid_I && valid_W && valid_T && valid_Ta && valid_Tm && valid_Tm )
        {
            time = ftemp / I / I * A * A / ONDERKONK_COEFF;
            m_timeValue->SetValue( wxString::Format( wxT( "%f" ), time ) );
        }
        else
        {
            m_timeValue->SetValue( "Error" );
        }
    }
    else
    {
        // What happened ??? an extra radio button ?
    }

    // Now let's check the validity domain using the formula from the paper.
    // https://adam-research.de/pdfs/TRM_WhitePaper10_AdiabaticWire.pdf
    // We approximate the track with a circle having the same area.

    double cp      = 385;                     // Heat capacity in J / kg / K
    double rho     = 3900;                    // Mass density in kg / m^3
    double r       = sqrt( A / 3.14 ) / 1000; //radius in m;
    double epsilon = 5.67e-8;                 // Stefan-Boltzmann constant in W / ( m^2 K^4 )
    double sigma   = 0.5;                     // Surface radiative emissivity ( no unit )
    // sigma is according to paper, between polished and oxidized

    double tmKelvin = Tm + 273;
    double frad = 0.5 * ( tmKelvin + 293 ) * ( tmKelvin + 293 ) * ( tmKelvin + 293 );

    double tau = cp * rho * r / ( epsilon * sigma * frad * 2 );

    if( 2 * time < tau )
    {
        m_comment->SetLabel( "" );
    }
    else
    {
        m_comment->SetLabel( "Time is high compared to geometry: results might not be accurate. "
                             "The track could handle much more current" );
    }
}