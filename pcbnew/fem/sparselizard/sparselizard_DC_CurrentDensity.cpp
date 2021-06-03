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
#include "../gmsh_mesher.h"
#include "../common/fem_descriptor.h"


#define USE_GMSH

#define MIN_ORDER 1
#define MAX_ORDER 3


std::vector<int> getAllRegionsWithNetcode( std::map<int, int> aRegionMap, int aNetCode,
                                           int aIgnoredPort = -1 )
{
    std::vector<int> regions;

    for( std::map<int, int>::iterator it = aRegionMap.begin(); it != aRegionMap.end(); ++it )
    {
        if( ( it->second == aNetCode ) && ( it->first != aIgnoredPort ) )
        {
            regions.push_back( it->first );
        }
    }
    return regions;
}


double compute_DC_Current( expression j, int aPort, std::map<int, int> aRegionMap, int aNetCode )
{
    std::vector<int> regions = getAllRegionsWithNetcode( aRegionMap, aNetCode, aPort );

    if( regions.size() < 1 )
    {
        // The port is the only region with aNetCode, it is not connected to anything
        std::cerr << "Port is not connected to any region" << std::endl;
        return 0;
    }
    int outsideOfPort = sl::selectunion( regions );
    int line = sl::selectintersection( { aPort, outsideOfPort } );
    return ( sl::normal( aPort ) * sl::on( outsideOfPort, j ) ).integrate( line, 4 );
}


double compute_DC_Voltage( expression v, int aPortA, int aPortB )
{
    double result;
    result = v.integrate( aPortA, 4 ) / expression( 1 ).integrate( aPortA, 4 );
    result -= v.integrate( aPortB, 4 ) / expression( 1 ).integrate( aPortB, 4 );
    return result;
}

double compute_DC_Potential( expression v, int aPortA )
{
    double result;
    result = v.integrate( aPortA, 4 ) / expression( 1 ).integrate( aPortA, 4 );
    return result;
}

double compute_DC_Resistance( expression v, expression j, int aPortA, int aPortB,
                              std::map<int, int> aRegionMap, int aNetCode )
{
    double V, I;
    V = compute_DC_Voltage( v, aPortA, aPortB );
    I = compute_DC_Current( j, aPortA, aRegionMap, aNetCode );

    if( ( I == 0 ) && ( V == 0 ) )
        return 0; // Should be nan, we could not get the resistance
    if( I == 0 )
        return std::numeric_limits<double>::infinity();

    return V / I;
}

double compute_DC_Power( expression v, expression j, int aPortA, int aPortB,
                         std::map<int, int> aRegionMap, int aNetCode )
{
    double V, I;
    V = compute_DC_Voltage( v, aPortA, aPortB );
    I = compute_DC_Current( j, aPortA, aRegionMap, aNetCode );

    return V * I;
}

bool Run_DC_CurrentDensity( FEM_DESCRIPTOR* aDescriptor )
{
    const double copperResistivity = 1.68e-8 / ( 35e-6 );
    const double interpolationOrder = 1;

    if( aDescriptor == nullptr )
        return false;

    if( aDescriptor->GetBoard() == nullptr )
        return false;

#ifdef USE_GMSH
    GMSH_MESHER mesher( aDescriptor->GetBoard() );
#else
    SPARSELIZARD_MESHER mesher( aDescriptor->GetBoard() );
#endif
    std::cout << "Trying to add ports" << std::endl;

    std::list<int> netlist;
    std::map<int, int> regionMap; // RegionID, netcode

    for( FEM_PORT* port : aDescriptor->GetPorts() )
    {
        if( port->GetItem() == nullptr )
        {
            std::cerr << "Uninitialized port" << std::endl;
            continue;
        }

        std::list<int>::iterator it;
        it = std::find( netlist.begin(), netlist.end(), port->GetItem()->GetNetCode() );
        if( it == netlist.end() )
        {
            netlist.push_back( port->GetItem()->GetNetCode() );
        }

        switch( port->GetItem()->Type() )
        {
        case PCB_PAD_T:
            port->m_simulationID =
                    mesher.AddPadRegion( static_cast<const PAD*>( port->GetItem() ) );
            regionMap.insert(
                    std::make_pair( port->m_simulationID,
                                    static_cast<const PAD*>( port->GetItem() )->GetNetCode() ) );
            break;
        default: break;
        }
    }

    // Add copper zones / tracks

    for( int netID : netlist )
    {
        int regionId = mesher.AddNetRegion( netID );
        regionMap.insert( std::make_pair( regionId, netID ) );
    }

    std::cout << "Number of ports in simulation: " << regionMap.size() - netlist.size()
              << std::endl;
    std::cout << "Number of regions in simulation: " << regionMap.size() << std::endl;


    std::cout << std::endl << "---------SPARSELIZARD---------" << std::endl;

#ifdef USE_GMSH
    mesher.Load25DMesh();
    mesh mymesh( "gmsh:api" );
    mesher.Finalize();
#else
    std::vector<shape> shapes;
    mesher.Get2DShapes( shapes, PCB_LAYER_ID::F_Cu, true );
    mesh mymesh;
    mymesh.split( 2 );
    mymesh.load( shapes );
#endif

    mymesh.write( "mymesh.msh" );


    std::cout << std::endl << "---------SIMULATION---------" << std::endl;
    // Create the electric potential field v
    field v( "h1" );
    // parameters
    parameter   rho; // resistivity
    formulation equations;

    for( std::map<int, int>::iterator it = regionMap.begin(); it != regionMap.end(); ++it )
    {
        v.setorder( it->first, interpolationOrder );
        rho | it->first = copperResistivity;
    }

    for( FEM_PORT* portA : aDescriptor->GetPorts() )
    {
        if( portA->GetItem() == nullptr )
        {
            std::cerr << "Uninitialized port" << std::endl;
            continue;
        }

        if( portA->m_type != FEM_PORT_TYPE::PASSIVE ) // Don't constrain passive ports
        {
            port V, I;
            switch( portA->m_constraint.m_type )
            {
            case FEM_PORT_CONSTRAINT_TYPE::VOLTAGE:
                v.setport( portA->m_simulationID, V, I );
                equations += V - portA->m_constraint.m_value;
                std::cout << "Setting region " << portA->m_simulationID << " to "
                          << portA->m_constraint.m_value << " V" << std::endl;
                break;
            case FEM_PORT_CONSTRAINT_TYPE::CURRENT:
            {
                v.setport( portA->m_simulationID, V, I );
                equations += I - portA->m_constraint.m_value;
                std::cout << "Setting region " << portA->m_simulationID << " divergence to "
                          << portA->m_constraint.m_value << " A" << std::endl;
                break;
            }
            default: std::cerr << "Source / Sink type not supported" << std::endl;
            }
        }
        else
        {
        }
    }

    for( std::map<int, int>::iterator it = regionMap.begin(); it != regionMap.end(); ++it )
    {
        equations +=
                sl::integral( it->first, sl::grad( sl::tf( v ) ) / rho * sl::grad( sl::dof( v ) ) );
    }

    // Expression for the electric field E [V/m] and current density j [A/m^2]:
    expression E = -sl::grad( v ); // electric field
    expression j = E / rho;        // current density
    expression p = sl::doubledotproduct( j, E ); // power density
    //expression P = sl::compx(p)*sl::compx(p)+sl::compy(p)*sl::compy(p);          // power density


    int  nb_refinements = 0;
    int  wholedomain = sl::selectall();
    bool need_adapt = false;

    // Simple p-adaptivity loop:
    v.setorder( sl::norm( sl::grad( v ) ), MIN_ORDER, MAX_ORDER + 1 );
    for( int i = 0; i <= MAX_ORDER - MIN_ORDER; i++ )
    {
        equations.solve( "cholesky" );
        need_adapt = sl::adapt();
        if( !need_adapt )
        {
            break; // No adaptation was needed
        };
    }


    sl::fieldorder( v ).write( wholedomain, "fieldOrder.pos" );

    std::cout << std::endl << "---------RESULTS---------" << std::endl;

    for( FEM_RESULT* result : aDescriptor->GetResults() )
    {
        if( result == nullptr )
        {
            std::cerr << "Uninitialized result - null" << std::endl;
            continue;
        }

        switch( result->GetType() )
        {
        case FEM_RESULT_TYPE::VALUE:
        {
            FEM_RESULT_VALUE* resultValue = static_cast<FEM_RESULT_VALUE*>( result );

            if( resultValue->IsInitialized() == false )
            {
                std::cerr << "Uninitialized result" << std::endl;
                continue;
            }
            std::cout << "--Result type : ";
            switch( resultValue->m_valueType )
            {
            case FEM_VALUE_TYPE::POTENTIAL:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                std::cout << "Potential at " << portA << endl;
                resultValue->m_value = compute_DC_Potential( v, portA );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::VOLTAGE:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                int portB = resultValue->GetPortB()->m_simulationID;
                std::cout << "Voltage from " << portB << " to " << portA << endl;
                resultValue->m_value = compute_DC_Voltage( v, portA, portB );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::CURRENT:
            {
                int port = resultValue->GetPortA()->m_simulationID;
                std::cout << "Current through " << port << endl;
                int netCode = resultValue->GetPortA()->GetItem()->GetNetCode();

                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    std::cerr << "Computing current on passive port "
                              << resultValue->GetPortA()->m_simulationID << ". Result is 0."
                              << std::endl;
                    // On passive port, all current that flows into the port, also flows out of the port
                    // Therefore the closed loop integral of the current density over is 0.
                    resultValue->m_value = 0;
                }
                else
                {
                    resultValue->m_value = compute_DC_Current( j, port, regionMap, netCode );
                }
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::RESISTANCE:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                int portB = resultValue->GetPortB()->m_simulationID;
                int netCode = resultValue->GetPortA()->GetItem()->GetNetCode();
                std::cout << "Resistance between " << portB << " and " << portA << endl;
                // Find the region with same potential as
                resultValue->m_value =
                        compute_DC_Resistance( v, j, portA, portB, regionMap, netCode );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::DISSIPATED_POWER:
            {
                int port = resultValue->GetPortA()->m_simulationID;
                std::cout << "Power dissipated by passive port " << port << endl;
                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    int port = resultValue->GetPortA()->m_simulationID;
                    resultValue->m_value = ( sl::on( port, p ) ).integrate( port, 4 );
                    resultValue->m_valid = true;
                }
                else
                {
                    std::cerr << " Copper a sink / source port does not dissipate power" << port
                              << endl;
                    resultValue->m_value = 0;
                    resultValue->m_valid = true;
                }
                break;
            }
            case FEM_VALUE_TYPE::POWER:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                int portB = resultValue->GetPortB()->m_simulationID;
                int netCode = resultValue->GetPortA()->GetItem()->GetNetCode();
                std::cout << "Total power on port " << portA << " using " << portB
                          << " as reference." << std::endl;
                // Find the region with same potential as

                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    std::cerr << " Cannot get power flowing through a passive port ( net current "
                                 "is 0 ) "
                              << std::endl;
                    resultValue->m_value = 0;
                    resultValue->m_valid = true;
                }
                else
                {
                    resultValue->m_value =
                            compute_DC_Power( v, j, portA, portB, regionMap, netCode );
                    resultValue->m_valid = true;
                }
                break;
            }
            default: std::cerr << "Result type not supported by DC simulation" << std::endl; break;
            }
        }
        break;
        case FEM_RESULT_TYPE::MESH: break;
        case FEM_RESULT_TYPE::VIEW:
        {
            FEM_RESULT_VIEW* resultView = static_cast<FEM_RESULT_VIEW*>( result );
            switch( resultView->m_viewType )
            {
            case FEM_VIEW_TYPE::TEMPERATURE:
                std::cerr << "Temperature is not supported by DC simulation" << std::endl;
                break;
            case FEM_VIEW_TYPE::CURRENT:

                j.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                         MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::VOLTAGE:
                v.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                         MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::POWER:
                p.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                         MAX_ORDER );
                resultView->m_valid = true;
                break;
            default:
                std::cerr << "Result type not supported by DC simulation" << std::endl;
                break;
            }
            if( resultView->m_valid )
            {
                std::cout << "Filed created: " << resultView->m_filename.GetFullName() << std::endl;
            }
        }
        break;
        default: std::cerr << "Result type not supported by DC simulation" << std::endl;
        }
    }
    cout << "---------END OF SPARSELIZARD---------" << std::endl << std::endl;
    return true;
}
