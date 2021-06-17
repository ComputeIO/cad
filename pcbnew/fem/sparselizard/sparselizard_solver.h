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

#define SPARSELIZARD_VERBOSITY 1

#include "../common/fem_descriptor.h"
#include "../common/fem_constants.h"
#include <sparselizard/sparselizard.h>
#include "../gmsh_mesher.h"
#include <reporter.h>

class SPARSELIZARD_DIELECTRIC
{
public:
    double epsilonr; // permittivity
    double lossTangent;
    double regionID;
};

class SPARSELIZARD_CONDUCTOR
{
public:
    KICAD_T   type;
    int    netCode;
    double rho; // resistivity
    int       regionID;
    int       boundaryID; // used for capacitances
    port   primalPort;
    port   dualPort;
    FEM_PORT* femPort;
};

class SPARSELIZARD_SOLVER
{
public:
    SPARSELIZARD_SOLVER();
    SPARSELIZARD_SOLVER( REPORTER* aReporter );

    bool Run_DC( FEM_DESCRIPTOR* aDescriptor );

    void SetRegions( FEM_DESCRIPTOR* aDescriptor, GMSH_MESHER* aMesher );
    void SetBoundaries();
    void simulate();

    double computeCurrentDC( SPARSELIZARD_CONDUCTOR aConA );
    double computeVoltageDC( SPARSELIZARD_CONDUCTOR aConA, SPARSELIZARD_CONDUCTOR aConB );
    double computePotentialDC( SPARSELIZARD_CONDUCTOR aConA );
    double computeResistanceDC( SPARSELIZARD_CONDUCTOR aConA, SPARSELIZARD_CONDUCTOR aConB );
    double computePowerDC( SPARSELIZARD_CONDUCTOR aConA, SPARSELIZARD_CONDUCTOR aConB );
    double computeCapacitanceDC( SPARSELIZARD_CONDUCTOR aConA, SPARSELIZARD_CONDUCTOR aConB );

    void setVoltageDC( SPARSELIZARD_CONDUCTOR aCon, double aV );
    void setCurrentDC( SPARSELIZARD_CONDUCTOR aCon, double aI );
    void setChargeDC( SPARSELIZARD_CONDUCTOR aCon, double aQ );

    std::vector<int> getAllRegionsWithNetcode( int aNetCode, int aIgnoredPort );
    std::vector<int> getAllRegionsWithNetcode( int aNetCode );


    SPARSELIZARD_CONDUCTOR* findConductor( FEM_PORT* aPort );

    void setEquations( FEM_DESCRIPTOR* aDescriptor );
    void setConstraints( FEM_DESCRIPTOR* aDescriptor );
    bool setParameters( FEM_DESCRIPTOR* aDescriptor );
    void writeResults( FEM_DESCRIPTOR* aDescriptor );

    int m_boundary;
    // Holds all equations
    formulation *m_equations; // For some reasons, this lead to a segfault on constructor if not using a pointer

    // Electric potential field
    field m_v;
    // Magnetic vector potential field
    field m_A;
    // Magnetic field
    // Expressions, derived from the electric potential field
    expression m_E; // Electriec field
    expression m_j; // current density
    expression m_p; // power density
    // Expressions, derived from the magnetic field
    expression m_B; // Magnetic flux density
    expression m_H; // Magnetic field

    // Parameters
    parameter* m_rho;     // resistivity
    parameter* m_epsilon; // permittivity
    parameter* m_mu;      // permeability

    REPORTER* m_reporter;

    std::vector<SPARSELIZARD_DIELECTRIC*> m_dielectrics;
    std::vector<SPARSELIZARD_CONDUCTOR*>  m_conductors;

    FEM_CONSTANTS m_constants;

    std::vector<int> m_dielectricRegions;
    std::vector<int> m_conductorRegions;

private:
};
#endif