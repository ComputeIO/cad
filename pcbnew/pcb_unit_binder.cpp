/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Reece R. Pollack <reece@his.com>
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

#include <common.h>
#include <pcbnew.h>
#include <pcb_base_frame.h>
#include <pcb_unit_binder.h>

#include <wx/debug.h>   // for wxASSERT

// DEVELOPMENT ONLY
#include <iostream>
using namespace std;

PCB_UNIT_BINDER::PCB_UNIT_BINDER( PCB_BASE_FRAME* aParent,
                                  wxStaticText* aLabel,
                                  wxWindow* aValue,
                                  wxStaticText* aUnitLabel,
                                  bool aUseMils, bool allowEval ) :
        UNIT_BINDER( aParent, aLabel, aValue, aUnitLabel, aUseMils, allowEval ),
        m_transformType( NULL_XFORM ),
        m_originTransforms( *aParent )
{
    wxASSERT( aParent != nullptr );
}

PCB_UNIT_BINDER::~PCB_UNIT_BINDER()
{}



long long int PCB_UNIT_BINDER::xformForDisplay( long long int aValue ) const
{
    long long int value = aValue;

    switch (m_transformType)
    {
    case ABS_X_COORD: value = m_originTransforms.ToDisplayAbsX( value );  break;
    case ABS_Y_COORD: value = m_originTransforms.ToDisplayAbsY( value );  break;
    case REL_X_COORD: value = m_originTransforms.ToDisplayRelX( value );  break;
    case REL_Y_COORD: value = m_originTransforms.ToDisplayRelY( value );  break;
    case NULL_XFORM:
    default:
        /* do nothing */
        break;
    };

    cout << __func__ << " type " << TransformType() << " from " << aValue << " to " << value << endl;
    return value;
}

double PCB_UNIT_BINDER::xformForDisplay( double aValue ) const
{
    double value = aValue;

    switch (m_transformType)
    {
    case ABS_X_COORD: value = m_originTransforms.ToDisplayAbsX( value );  break;
    case ABS_Y_COORD: value = m_originTransforms.ToDisplayAbsY( value );  break;
    case REL_X_COORD: value = m_originTransforms.ToDisplayRelX( value );  break;
    case REL_Y_COORD: value = m_originTransforms.ToDisplayRelY( value );  break;
    case NULL_XFORM:
    default:
        /* do nothing */
        break;
    };

    cout << __func__ << " type " << TransformType() << " from " << aValue << " to " << value << endl;
    return value;
}

long long PCB_UNIT_BINDER::xformFromInput( long long aValue ) const
{
    long long value = aValue;

    switch (m_transformType)
    {
    case ABS_X_COORD: value = m_originTransforms.FromDisplayAbsX( value );  break;
    case ABS_Y_COORD: value = m_originTransforms.FromDisplayAbsY( value );  break;
    case REL_X_COORD: value = m_originTransforms.FromDisplayRelX( value );  break;
    case REL_Y_COORD: value = m_originTransforms.FromDisplayRelY( value );  break;
    case NULL_XFORM:
    default:
        /* do nothing */
        break;
    };

    cout << __func__ << " type " << TransformType() << " from " << aValue << " to " << value << endl;
    return value;
}

double PCB_UNIT_BINDER::xformFromInput( double aValue ) const
{
    double value = aValue;

    switch (m_transformType)
    {
    case ABS_X_COORD: value = m_originTransforms.FromDisplayAbsX( value );  break;
    case ABS_Y_COORD: value = m_originTransforms.FromDisplayAbsY( value );  break;
    case REL_X_COORD: value = m_originTransforms.FromDisplayRelX( value );  break;
    case REL_Y_COORD: value = m_originTransforms.FromDisplayRelY( value );  break;
    case NULL_XFORM:
    default:
        /* do nothing */
        break;
    };

    cout << __func__ << " type " << TransformType() << " from " << aValue << " to " << value << endl;
    return value;
}

void PCB_UNIT_BINDER::SetValue( int aValue )
{
    long long value = aValue;
    int displayValue = xformForDisplay( value );
    UNIT_BINDER::SetValue( displayValue );
}

void PCB_UNIT_BINDER::SetDoubleValue( double aValue )
{
    double displayValue = xformForDisplay( aValue );
    UNIT_BINDER::SetDoubleValue( displayValue );
}

void PCB_UNIT_BINDER::ChangeValue( int aValue )
{
    long long value = aValue;
    int displayValue = xformForDisplay( value );
    UNIT_BINDER::ChangeValue( displayValue );
}

long long int PCB_UNIT_BINDER::GetValue()
{
    long long int displayValue = UNIT_BINDER::GetValue();
    return xformFromInput( displayValue );
}

double PCB_UNIT_BINDER::GetDoubleValue()
{
    double displayValue = UNIT_BINDER::GetDoubleValue();
    return xformFromInput( displayValue );
}
