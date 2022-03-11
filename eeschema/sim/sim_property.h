/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SIM_PROPERTY_H
#define SIM_PROPERTY_H

#include <sim/sim_value.h>
#include <wx/propgrid/props.h>


class SIM_VALIDATOR : public wxTextValidator
{
public:
    wxString IsValid( const wxString& aString ) const override;
    bool Validate( wxWindow* aParent ) override;

protected:
    SIM_VALIDATOR();
};

class SIM_INT_VALIDATOR : public SIM_VALIDATOR
{
public:
    SIM_INT_VALIDATOR( SIM_VALUE_PARSER::NOTATION aNotation );
};

class SIM_FLOAT_VALIDATOR : public SIM_VALIDATOR
{
public:
    SIM_FLOAT_VALIDATOR( SIM_VALUE_PARSER::NOTATION aNotation );
};


class SIM_PROPERTY : public wxStringProperty
{
public:
    SIM_PROPERTY( const wxString& aLabel, const wxString& aName, SIM_VALUE_BASE& aValue,
                  SIM_VALUE_PARSER::NOTATION aNotation = SIM_VALUE_PARSER::NOTATION::SI );

    wxValidator* DoGetValidator() const override = 0;

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 )
        const override;

protected:
    SIM_VALUE_PARSER::NOTATION m_notation;
    SIM_VALUE_BASE& m_value;
};

class SIM_INT_PROPERTY : public SIM_PROPERTY
{
    using SIM_PROPERTY::SIM_PROPERTY;

    wxValidator* DoGetValidator() const override;
};

class SIM_FLOAT_PROPERTY : public SIM_PROPERTY
{
    using SIM_PROPERTY::SIM_PROPERTY;

    wxValidator* DoGetValidator() const override;
};

#endif // SIM_PROPERTY_H
