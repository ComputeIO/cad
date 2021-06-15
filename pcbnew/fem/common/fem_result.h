/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FEM_RESULT_H
#define FEM_RESULT_H

#include "fem_port.h"
#include <wx/filename.h>

enum class FEM_RESULT_TYPE
{
    VALUE,
    FREQUENCY_SWEEP,
    MESH,
    VIEW
};

class FEM_RESULT
{
public:
    FEM_RESULT( FEM_RESULT_TYPE aType );
    bool            m_valid;
    bool            IsInitialized() { return m_initialized; };
    FEM_RESULT_TYPE GetType() { return m_type; };

private:
    // Two ports should be enough for any measurement ?
    FEM_RESULT_TYPE m_type;
    bool            m_initialized;
};

enum class FEM_VALUE_TYPE
{
    TEMPERATURE,
    POTENTIAL,
    CURRENT,
    DISSIPATED_POWER, // Power dissipated by a single port ! From a field expression, no P = U * I
    VOLTAGE,
    POWER,
    RESISTANCE,
    CAPACITANCE,
    INDUCTANCE,
    REACTANCE,
    NB_TYPE,
    REQUIRES_2_PORTS = VOLTAGE
};


class FEM_RESULT_VALUE : public FEM_RESULT
{
public:
    FEM_RESULT_VALUE( FEM_VALUE_TYPE aType, FEM_PORT* aPortA, FEM_PORT* aPortB );

    bool            m_valid;
    double          GetResult() { return m_value; };
    bool            IsInitialized() { return m_initialized; };
    FEM_RESULT_TYPE GetType() { return m_type; };
    FEM_VALUE_TYPE  m_valueType;
    FEM_PORT*       GetPortA() { return m_portA; };
    FEM_PORT*       GetPortB() { return m_portB; };
    double          m_value;

private:
    // Two ports should be enough for any measurement ?
    FEM_PORT *      m_portA, *m_portB;
    FEM_RESULT_TYPE m_type;
    bool            m_initialized;
};


enum class FEM_VIEW_TYPE
{
    // Show in copper only
    TEMPERATURE,
    CURRENT,
    VOLTAGE,
    POWER,
    // Show in dielectric only
    ELECTRIC_FIELD,
    NB_TYPE,
};

class FEM_RESULT_VIEW : public FEM_RESULT
{
public:
    FEM_RESULT_VIEW( FEM_VIEW_TYPE aType, wxFileName aFilename );

    bool            m_valid;
    wxFileName      GetResult() { return m_filename; };
    bool            IsInitialized() { return m_initialized; };
    FEM_RESULT_TYPE GetType() { return m_type; };
    FEM_VIEW_TYPE   m_viewType;
    wxFileName      m_filename;

private:
    // Two ports should be enough for any measurement ?
    FEM_RESULT_TYPE m_type;
    bool            m_initialized;
};
#endif