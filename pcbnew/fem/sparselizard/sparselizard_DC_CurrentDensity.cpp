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

SPARSELIZARD_SOLVER::SPARSELIZARD_SOLVER()
{
    m_reporter = new STDOUT_REPORTER();
}

SPARSELIZARD_SOLVER::SPARSELIZARD_SOLVER( REPORTER* aReporter )
{
    if( aReporter == nullptr )
    {
        m_reporter = new STDOUT_REPORTER();
    }
    else
    {
        m_reporter = aReporter;
    }
}


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


double SPARSELIZARD_SOLVER::computeCurrentDC( int aPort, std::map<int, int> aRegionMap,
                                              int aNetCode )
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
    return ( sl::normal( aPort ) * sl::on( outsideOfPort, m_j ) ).integrate( line, 4 );
}


double SPARSELIZARD_SOLVER::computeVoltageDC( int aPortA, int aPortB )
{
    double result;
    result = m_v.integrate( aPortA, 4 ) / expression( 1 ).integrate( aPortA, 4 );
    result -= m_v.integrate( aPortB, 4 ) / expression( 1 ).integrate( aPortB, 4 );
    return result;
}

double SPARSELIZARD_SOLVER::computePotentialDC( int aPortA )
{
    double result;
    result = m_v.integrate( aPortA, 4 ) / expression( 1 ).integrate( aPortA, 4 );
    return result;
}

double SPARSELIZARD_SOLVER::computeResistanceDC( int aPortA, int aPortB,
                                                 std::map<int, int> aRegionMap, int aNetCode )
{
    double V, I;
    V = computeVoltageDC( aPortA, aPortB );
    I = computeCurrentDC( aPortA, aRegionMap, aNetCode );

    if( ( I == 0 ) && ( V == 0 ) )
        return 0; // Should be nan, we could not get the resistance
    if( I == 0 )
        return std::numeric_limits<double>::infinity();

    return V / I;
}

double SPARSELIZARD_SOLVER::computePowerDC( int aPortA, int aPortB, std::map<int, int> aRegionMap,
                                            int aNetCode )
{
    double V, I;
    V = computeVoltageDC( aPortA, aPortB );
    I = computeCurrentDC( aPortA, aRegionMap, aNetCode );

    return V * I;
}

void SPARSELIZARD_SOLVER::setVoltageDC( int aRegion, double aV )
{
    port V, I;
    m_v.setport( aRegion, V, I );
    ( *m_equations ) += V - aV;
}

void SPARSELIZARD_SOLVER::setCurrentDC( int aRegion, double aI )
{
    port V, I;
    m_v.setport( aRegion, V, I );
    ( *m_equations ) += I - aI;
}


bool SPARSELIZARD_SOLVER::Run_DC( FEM_DESCRIPTOR* aDescriptor )
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
    m_reporter->Report( "Setting up ports and regions...", RPT_SEVERITY_INFO );

    std::list<int> netlist;
    std::map<int, int> regionMap; // RegionID, netcode

    for( FEM_PORT* port : aDescriptor->GetPorts() )
    {
        if( port->GetItem() == nullptr )
        {
            m_reporter->Report( "A FEM_PORT is NULL", RPT_SEVERITY_ERROR );
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

    m_reporter->Report( "Number of ports in simulation: "
                                + to_string( ( regionMap.size() - netlist.size() ) ),
                        RPT_SEVERITY_INFO );
    m_reporter->Report( "Number of regions in simulation: " + to_string( regionMap.size() ),
                        RPT_SEVERITY_INFO );

    m_reporter->Report( "SPARSELIZARD: Loading mesh...", RPT_SEVERITY_ACTION );

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

    m_reporter->Report( "SPARSELIZARD: mesh loaded.", RPT_SEVERITY_INFO );

    mymesh.write( "mymesh.msh" );

    m_reporter->Report( "Setting up simulation ports...", RPT_SEVERITY_ACTION );
    // Create the electric potential field v
    m_v = field( "h1" );
    formulation eq;
    m_equations = &eq;
    // parameters
    parameter   rho; // resistivity

    for( std::map<int, int>::iterator it = regionMap.begin(); it != regionMap.end(); ++it )
    {
        m_v.setorder( it->first, interpolationOrder );
        rho | it->first = copperResistivity;
    }

    for( FEM_PORT* portA : aDescriptor->GetPorts() )
    {
        if( portA->GetItem() == nullptr )
        {
            m_reporter->Report( "A FEM_PORT is NULL", RPT_SEVERITY_ERROR );
            continue;
        }

        if( portA->m_type != FEM_PORT_TYPE::PASSIVE ) // Don't constrain passive ports
        {
            port V, I;
            switch( portA->m_constraint.m_type )
            {
            case FEM_PORT_CONSTRAINT_TYPE::VOLTAGE:
                setVoltageDC( portA->m_simulationID, portA->m_constraint.m_value );
                break;
            case FEM_PORT_CONSTRAINT_TYPE::CURRENT:
            {
                setCurrentDC( portA->m_simulationID, portA->m_constraint.m_value );
                break;
            }
            default: m_reporter->Report( "Source / Sink type not supported", RPT_SEVERITY_ERROR );
            }
        }
        else
        {
        }
    }

    for( std::map<int, int>::iterator it = regionMap.begin(); it != regionMap.end(); ++it )
    {
        ( *m_equations ) += sl::integral( it->first, sl::grad( sl::tf( m_v ) ) / rho
                                                             * sl::grad( sl::dof( m_v ) ) );
    }

    // Expression for the electric field E [V/m] and current density j [A/m^2]:
    m_E = -sl::grad( m_v );                 // electric field
    m_j = m_E / rho;                        // current density
    m_p = sl::doubledotproduct( m_j, m_E ); // power density


    int  nb_refinements = 0;
    int  wholedomain = sl::selectall();
    bool need_adapt = false;

    // Simple p-adaptivity loop:
    m_v.setorder( sl::norm( sl::grad( m_v ) ), MIN_ORDER, MAX_ORDER + 1 );


    m_reporter->Report( "SPARSELIZARD: Simulating...", RPT_SEVERITY_ACTION );

    for( int i = 0; i <= MAX_ORDER - MIN_ORDER; i++ )
    {
        m_equations->solve( "cholesky" );
        need_adapt = sl::adapt();
        if( !need_adapt )
        {
            break; // No adaptation was needed
        };
    }

    m_reporter->Report( "SPARSELIZARD: End of simulation.", RPT_SEVERITY_INFO );

    sl::fieldorder( m_v ).write( wholedomain, "fieldOrder.pos" );

    for( FEM_RESULT* result : aDescriptor->GetResults() )
    {
        if( result == nullptr )
        {
            m_reporter->Report( "A FEM_RESULT is NULL", RPT_SEVERITY_ERROR );
            ;
            continue;
        }

        switch( result->GetType() )
        {
        case FEM_RESULT_TYPE::VALUE:
        {
            FEM_RESULT_VALUE* resultValue = static_cast<FEM_RESULT_VALUE*>( result );

            if( resultValue->IsInitialized() == false )
            {
                m_reporter->Report( "A result is unitialized", RPT_SEVERITY_ERROR );
                ;
                continue;
            }
            switch( resultValue->m_valueType )
            {
            case FEM_VALUE_TYPE::POTENTIAL:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                resultValue->m_value = computePotentialDC( portA );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::VOLTAGE:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                int portB = resultValue->GetPortB()->m_simulationID;
                resultValue->m_value = computeVoltageDC( portA, portB );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::CURRENT:
            {
                int port = resultValue->GetPortA()->m_simulationID;
                int netCode = resultValue->GetPortA()->GetItem()->GetNetCode();

                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    m_reporter->Report( "Cannot get current flowing through a passive port ( net "
                                        "current is 0 )",
                                        RPT_SEVERITY_ERROR );
                    // On passive port, all current that flows into the port, also flows out of the port
                    // Therefore the closed loop integral of the current density over is 0.
                    resultValue->m_value = 0;
                }
                else
                {
                    resultValue->m_value = computeCurrentDC( port, regionMap, netCode );
                }
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::RESISTANCE:
            {
                int portA = resultValue->GetPortA()->m_simulationID;
                int portB = resultValue->GetPortB()->m_simulationID;
                int netCode = resultValue->GetPortA()->GetItem()->GetNetCode();
                resultValue->m_value = computeResistanceDC( portA, portB, regionMap, netCode );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::DISSIPATED_POWER:
            {
                int port = resultValue->GetPortA()->m_simulationID;
                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    resultValue->m_value = ( sl::on( port, m_p ) ).integrate( port, 4 );
                    resultValue->m_valid = true;
                }
                else
                {
                    m_reporter->Report( "Copper on a sink / source port does not dissipate power",
                                        RPT_SEVERITY_ERROR );
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

                if( resultValue->GetPortA()->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    m_reporter->Report(
                            "Cannot get power flowing through a passive port ( net current is 0 )",
                            RPT_SEVERITY_ERROR );
                    resultValue->m_value = 0;
                    resultValue->m_valid = true;
                }
                else
                {
                    resultValue->m_value = computePowerDC( portA, portB, regionMap, netCode );
                    resultValue->m_valid = true;
                }
                break;
            }
            default:
                m_reporter->Report( "Result type is not supported by DC simulation",
                                    RPT_SEVERITY_ERROR );
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
                m_reporter->Report( "Temperature is not supported by DC simulation",
                                    RPT_SEVERITY_ERROR );
                break;
            case FEM_VIEW_TYPE::CURRENT:
                m_j.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                           MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::VOLTAGE:
                m_v.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                           MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::POWER:
                m_p.write( wholedomain, std::string( resultView->m_filename.GetFullName() ),
                           MAX_ORDER );
                resultView->m_valid = true;
                break;
            default:
                m_reporter->Report( "Result type not supported by DC simulation",
                                    RPT_SEVERITY_ERROR );
                break;
            }
            if( resultView->m_valid )
            {
                m_reporter->Report( "Filed created: " + resultView->m_filename.GetFullName(),
                                    RPT_SEVERITY_ACTION );
            }
        }
        break;
        default:
            m_reporter->Report( "Result type not supported by DC simulation", RPT_SEVERITY_ERROR );
        }
    }
    m_reporter->Report( "Finished", RPT_SEVERITY_INFO );
    return true;
}
