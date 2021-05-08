/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include <sparselizard/sparselizard.h>
#include "sparselizard_solver.h"
#include "sparselizard_mesher.h"
#include "../common/fem_descriptor.h"

bool Run_DC_CurrentDensity( FEM_DESCRIPTOR* aDescriptor )
{
    if( aDescriptor == NULL )
        return false;

    if( aDescriptor->m_board == NULL )
        return false;

    SPARSELIZARD_MESHER mesher = SPARSELIZARD_MESHER( aDescriptor->m_board );
    std::cout << "Trying to add ports" << std::endl;

    std::list<int> netlist;
    std::list<int> regionlist;

    for( FEM_PORT* port : aDescriptor->GetPorts() )
    {
        if( port->m_item == NULL )
        {
            std::cerr << "Uninitialized port" << std::endl;
            continue;
        }

        std::list<int>::iterator it;
        it = std::find( netlist.begin(), netlist.end(), port->m_item->GetNetCode() );
        if( it == netlist.end() )
        {
            netlist.push_back( port->m_item->GetNetCode() );
        }

        switch( port->m_item->Type() )
        {
        case PCB_PAD_T:
            port->m_simulationID = mesher.AddPadRegion( static_cast<const PAD*>( port->m_item ) );
            break;
        default: break;
        }
        std::cout << "Port found" << std::endl;
    }

    // Add copper zones / tracks
    for( int netID : netlist )
    {
        int regionId = mesher.AddNetRegion( netID );
        regionlist.push_back( regionId );
    }

    std::cout << "Number of regions in simulation: " << netlist.size() << std::endl;


    std::vector<shape> shapes;

    mesher.Get2DShapes( shapes, PCB_LAYER_ID::F_Cu, true );

    mesh mymesh( shapes );

    mymesh.write( "mymesh.msh" );

    // from now - Sparselizard  only
    // Create the electric potential field v and temperature field T (both with nodal shape functions h1):

    std::cout << std::endl << "---------SPARSELIZARD---------" << netlist.size() << std::endl;

    field v( "h1" );
    // parameters
    parameter   rho; // resistivity
    formulation electrokinetics;

    for( FEM_PORT* port : aDescriptor->GetPorts() )
    {
        if( port->m_item == NULL )
        {
            std::cerr << "Uninitialized port" << std::endl;
            continue;
        }

        if( port->m_constraint->m_type != FEM_PORT_CONSTRAINT_TYPE::VOLTAGE )
        {
            std::cerr << "Contraint should be FEM_PORT_CONSTRAINT_TYPE::VOLTAGE" << std::endl;
            continue;
        }
        v.setorder( port->m_simulationID, 2 );
        v.setconstraint( port->m_simulationID, port->m_constraint->m_value );
        std::cout << "Setting region " << port->m_simulationID << " to "
                  << port->m_constraint->m_value << " V" << std::endl;
        rho | port->m_simulationID = 1.68e-8;
    }

    for( int region : regionlist )
    {
        v.setorder( region, 2 );
        rho | region = 1.68e-8;
        electrokinetics += sl::integral( region, sl::grad( sl::tf( v ) ) * 1 / rho
                                                         * sl::grad( sl::dof( v ) ) );
    }

    // Expression for the electric field E [V/m] and current density j [A/m^2]:
    expression E = -sl::grad( v );
    expression j = 1 / rho * E;


    // Compute the static current everywhere:
    electrokinetics.generate();
    // Get A and b to solve Ax = b:
    vec solv = sl::solve( electrokinetics.A(), electrokinetics.b() );
    // Transfer the data from the solution vector to the v field to be used for the heat equation below:
    int wholedomain = sl::selectall();
    v.setdata( wholedomain, solv );
    j.write( wholedomain, "currentdensity.pos" );


    return true;
}