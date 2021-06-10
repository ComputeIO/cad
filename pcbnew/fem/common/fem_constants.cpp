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

#include "fem_constants.h"

#define EPSILON0 8.8541878128e-12 //Vacuum permittivity
#define MU0 1.25663706212e-6      //Vacuum permeability
#define CELERITY 2.99792458e8;    //Speed of light in vacuum
#define RHO_CU 1.72e-8;           // Copper resistivity

FEM_CONSTANTS::FEM_CONSTANTS()
{
    m_epsilon0 = EPSILON0;
    m_mu0 = MU0;
    m_c = CELERITY;
    m_rho_Cu = RHO_CU;
    m_units = EDA_UNITS::UNSCALED;
}


bool FEM_CONSTANTS::SetUnitScaling( EDA_UNITS aUnits )
{
    switch( aUnits )
    {
    case EDA_UNITS::UNSCALED:
    case EDA_UNITS::MILLIMETRES: m_units = aUnits; return true;
    default: return false;
    }
}


double FEM_CONSTANTS::epsilon0()
{
    switch( m_units )
    {
    case EDA_UNITS::MILLIMETRES: return m_epsilon0 / 1000;
    default: return m_epsilon0;
    }
}

double FEM_CONSTANTS::mu0()
{
    switch( m_units )
    {
    case EDA_UNITS::MILLIMETRES: return m_mu0 / 1000;
    default: return m_mu0;
    }
}

double FEM_CONSTANTS::c()
{
    switch( m_units )
    {
    case EDA_UNITS::MILLIMETRES: return m_c * 1000;
    default: return m_c;
    }
}

double FEM_CONSTANTS::rho_Cu()
{
    switch( m_units )
    {
    case EDA_UNITS::MILLIMETRES: return m_rho_Cu * 1000;
    default: return m_rho_Cu;
    }
}