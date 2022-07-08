/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sim_plot_axis_horizontal.h"
#include "sim/spice_value.h"
#include <ki_exception.h>
#include <confirm.h>


SIM_PLOT_AXIS_HORIZONTAL::SIM_PLOT_AXIS_HORIZONTAL( wxWindow* aParent, wxString aQuantity,
                                                    double aLeft, double aTick, double aRight,
                                                    bool aLog ) :
        SIM_PLOT_AXIS_HORIZONTAL_BASE( aParent ),
        m_quantity( aQuantity ), m_left( aLeft ), m_right( aRight ), m_ticks( aTick ), m_log( aLog )
{
    m_quantityCtrl->SetValue( m_quantity );
    m_quantityCtrl->Enable( false ); // for now. This needs more coding

    m_leftCtrl->SetValue( SPICE_VALUE( m_left ).ToSpiceString() );
    m_rightCtrl->SetValue( SPICE_VALUE( m_right ).ToSpiceString() );

    if( m_log )
    {
        m_ticksCtrl->SetValue( "--NA--" );
        m_ticksCtrl->Enable( false );
    }
    else
    {
        m_ticksCtrl->SetValue( SPICE_VALUE( m_ticks ).ToSpiceString() );
        m_ticksCtrl->Enable( true );
    }

    m_logCheckbox->SetValue( m_log );
}


void SIM_PLOT_AXIS_HORIZONTAL::OnOKButtonClick( wxCommandEvent& event )
{
    m_log = m_logCheckbox->GetValue();

    try
    {
        m_left = SPICE_VALUE( m_leftCtrl->GetValue() ).ToDouble();
        m_right = SPICE_VALUE( m_rightCtrl->GetValue() ).ToDouble();
    }
    catch( const KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( this, e.What() );
        return;
    }

    if( std::isnan( m_left ) || std::isnan( m_right ) )
    {
        DisplayErrorMessage( this, _( "Axis limit is not a number." ) );
        return;
    }

    if( m_right <= m_left )
    {
        DisplayErrorMessage( this, _( "The right value is lower than the left value." ) );
        return;
    }

    if( m_log && ( m_left < 0 ) )
    {
        DisplayErrorMessage( this, _( "Logarithmic axes require strictly positive values." ) );
        return;
    }


    event.Skip();
}