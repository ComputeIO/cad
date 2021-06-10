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

#ifndef FEM_CONSTANTS_H
#define FEM_CONSTANTS_H

#include <eda_units.h>

class FEM_CONSTANTS
{
public:
    FEM_CONSTANTS();
    double epsilon0(); //Vacuum permittivity
    double mu0();      //Vacuum permeability
    double c();        //Speed of light in vacuum
    double rho_Cu();   // Copper resistivity

    bool SetUnitScaling( EDA_UNITS aUnits );

private:
    EDA_UNITS m_units;

    double m_epsilon0;
    double m_mu0;
    double m_c;
    double m_rho_Cu;
};

#endif