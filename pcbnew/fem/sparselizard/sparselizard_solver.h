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

#ifndef KICAD_SPARSELIZARD_SOLVER_H
#define KICAD_SPARSELIZARD_SOLVER_H

#define SPARSELIZARD_VERBOSITY 0

#include "../common/fem_descriptor.h"
#include <sparselizard/sparselizard.h>
#include <reporter.h>

class SPARSELIZARD_DIELECTRIC
{
public:
    double epsilonr; // permittivity
    double regionID;
};

class SPARSELIZARD_CONDUCTOR
{
public:
    int    netCode;
    double rho; // resistivity
    double regionID;
    port   primalPort;
    port   dualPort;
};

class SPARSELIZARD_SOLVER
{
public:
    SPARSELIZARD_SOLVER();
    SPARSELIZARD_SOLVER( REPORTER* aReporter );

    bool Run_DC( FEM_DESCRIPTOR* aDescriptor );

    double computeCurrentDC( int aPort, int aNetCode );
    double computeVoltageDC( int aPortA, int aPortB );
    double computePotentialDC( int aPortA );
    double computeResistanceDC( int aPortA, int aPortB, int aNetCode );
    double computePowerDC( int aPortA, int aPortB, int aNetCode );
    double computeCapacitanceDC( int aPortA, int aPortB );

    void setVoltageDC( int aRegion, double aV );
    void setCurrentDC( int aRegion, double aI );
    void setChargeDC( int aRegion, double aQ );

    std::vector<int> getAllRegionsWithNetcode( int aNetCode, int aIgnoredPort );
    std::vector<int> getAllRegionsWithNetcode( int aNetCode );


    void setEquations();
    void setConstraints( FEM_DESCRIPTOR* aDescriptor );
    bool setParameters( FEM_DESCRIPTOR* aDescriptor );
    void writeResults( FEM_DESCRIPTOR* aDescriptor );
    // Holds all equations
    formulation *m_equations; // For some reasons, this lead to a segfault on constructor if not using a pointer

    // Electric potential field
    field m_v;
    // Expressions, derived from the electric potential field
    expression m_E; // Electriec field
    expression m_j; // current density
    expression m_p; // power density
    // Parameters
    parameter* m_rho;     // resistivity
    parameter* m_epsilon; // permittivity

    REPORTER* m_reporter;

    std::vector<SPARSELIZARD_DIELECTRIC> m_dielectrics;
    std::vector<SPARSELIZARD_CONDUCTOR>  m_conductors;


private:
};
#endif