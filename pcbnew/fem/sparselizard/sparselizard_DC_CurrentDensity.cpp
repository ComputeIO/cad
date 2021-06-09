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
#define EPSILON0 8.8541878128e-12 / 1000

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


std::vector<int> SPARSELIZARD_SOLVER::getAllRegionsWithNetcode( int aNetCode,
                                                                int aIgnoredPort = -1 )
{
    std::vector<int> regions;

    for( SPARSELIZARD_CONDUCTOR conductor : m_conductors )
    {
        if( ( conductor.netCode == aNetCode ) && ( conductor.regionID != aIgnoredPort ) )
        {
            regions.push_back( conductor.regionID );
        }
    }
    return regions;
}


double SPARSELIZARD_SOLVER::computeCurrentDC( SPARSELIZARD_CONDUCTOR aCon )
{
    port I;
    I = aCon.dualPort;
    return I.getvalue();
}


double SPARSELIZARD_SOLVER::computeVoltageDC( SPARSELIZARD_CONDUCTOR aConA,
                                              SPARSELIZARD_CONDUCTOR aConB )
{
    return computePotentialDC( aConA ) - computePotentialDC( aConB );
}

double SPARSELIZARD_SOLVER::computePotentialDC( SPARSELIZARD_CONDUCTOR aCon )
{
    port V;
    V = aCon.primalPort;
    return V.getvalue();
}

double SPARSELIZARD_SOLVER::computeResistanceDC( SPARSELIZARD_CONDUCTOR aConA,
                                                 SPARSELIZARD_CONDUCTOR aConB )
{
    double V, I;
    V = computeVoltageDC( aConA, aConB );
    I = computeCurrentDC( aConB );

    if( ( I == 0 ) && ( V == 0 ) )
        return 0; // Should be nan, we could not get the resistance
    if( I == 0 )
        return std::numeric_limits<double>::infinity();

    return V / I;
}

double SPARSELIZARD_SOLVER::computePowerDC( SPARSELIZARD_CONDUCTOR aConA,
                                            SPARSELIZARD_CONDUCTOR aConB )
{
    double V, I;
    V = computeVoltageDC( aConA, aConB );
    I = computeCurrentDC( aConA );

    return V * I;
}

double SPARSELIZARD_SOLVER::computeCapacitanceDC( SPARSELIZARD_CONDUCTOR aConA,
                                                  SPARSELIZARD_CONDUCTOR aConB )
{
    port QA, QB;
    QA = aConA.dualPort;
    QB = aConB.dualPort;
    double V = computeVoltageDC( aConA, aConB );
    double CA = QA.getvalue();
    double CB = QB.getvalue();
    std::cout << "total charges on A: " << CA << std::endl;
    std::cout << "total charges on B: " << CB << std::endl;
    std::cout << "Potential on A : " << aConA.primalPort.getvalue() << std::endl;
    std::cout << "Potential on B : " << aConB.primalPort.getvalue() << std::endl;
    return CA / V;
}

void SPARSELIZARD_SOLVER::setVoltageDC( SPARSELIZARD_CONDUCTOR aCon, double aV )
{
    port V, I;
    V = aCon.primalPort;
    I = aCon.dualPort;
    V.setvalue( aV );
    m_v.setport( aCon.regionID, V, I );
    //m_v.setconstraint( aCon.regionID, aV );
    ( *m_equations ) += V - aV;
}

void SPARSELIZARD_SOLVER::setCurrentDC( SPARSELIZARD_CONDUCTOR aCon, double aI )
{
    port V, I;
    V = aCon.primalPort;
    I = aCon.dualPort;
    m_v.setport( aCon.regionID, V, I );
    ( *m_equations ) += I - aI;
}

void SPARSELIZARD_SOLVER::setChargeDC( SPARSELIZARD_CONDUCTOR aCon, double aQ )
{
    port V, Q;
    V = aCon.primalPort;
    Q = aCon.dualPort;
    m_v.setport( aCon.regionID, V, Q );
    //( *m_equations ) += Q - aQ / ( expression( 1 ).integrate( aCon.regionID, 4 ) );
    ( *m_equations ) += Q - aQ;
    std::cout << "Setting total charge : " << aQ << std::endl;
}


void SPARSELIZARD_SOLVER::setConstraints( FEM_DESCRIPTOR* aDescriptor )
{
    for( FEM_PORT* portA : aDescriptor->GetPorts() )
    {
        if( portA->GetItem() == nullptr )
        {
            m_reporter->Report( "A FEM_PORT is NULL", RPT_SEVERITY_ERROR );
            continue;
        }


        if( portA->m_type != FEM_PORT_TYPE::PASSIVE ) // Don't constrain passive ports
        {
            SPARSELIZARD_CONDUCTOR region = findConductor( portA );
            if( region.regionID == 0 )
            {
                m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                continue;
            }

            switch( portA->m_constraint.m_type )
            {
            case FEM_PORT_CONSTRAINT_TYPE::VOLTAGE:
                setVoltageDC( region, portA->m_constraint.m_value );
                break;
            case FEM_PORT_CONSTRAINT_TYPE::CURRENT:
            {
                setCurrentDC( region, portA->m_constraint.m_value );
                break;
            }
            case FEM_PORT_CONSTRAINT_TYPE::CHARGE:
            {
                setChargeDC( region, portA->m_constraint.m_value );
                break;
            }
            default: m_reporter->Report( "Source / Sink type not supported", RPT_SEVERITY_ERROR );
            }
        }
        else
        {
        }
    }
}

bool SPARSELIZARD_SOLVER::setParameters( FEM_DESCRIPTOR* aDescriptor )
{
    for( SPARSELIZARD_DIELECTRIC dielectric : m_dielectrics )
    {
        m_v.setorder( dielectric.regionID, MIN_ORDER );
        ( *m_epsilon ) | dielectric.regionID = dielectric.epsilonr * EPSILON0;
        std::cout << "epsilon : " << ( dielectric.epsilonr * EPSILON0 ) << std::endl;
    }

    for( SPARSELIZARD_CONDUCTOR conductor : m_conductors )
    {
        m_v.setorder( conductor.regionID, MIN_ORDER );
        switch( aDescriptor->m_dim )
        {
        case FEM_SIMULATION_DIMENSION::SIMUL2D5:
            ( *m_rho ) | conductor.regionID = conductor.rho / ( 35e-3 );
            break;
        case FEM_SIMULATION_DIMENSION::SIMUL3D:
            ( *m_rho ) | conductor.regionID = conductor.rho;
            break;
        default:
            m_reporter->Report( "Simulation dimension is not supported", RPT_SEVERITY_ERROR );
            return false;
        }
    }
    return true;
}

void SPARSELIZARD_SOLVER::setEquations()
{
    for( SPARSELIZARD_CONDUCTOR conductor : m_conductors )
    {
        ( *m_equations ) +=
                sl::integral( conductor.regionID,
                              sl::grad( sl::tf( m_v ) ) / ( *m_rho ) * sl::grad( sl::dof( m_v ) ) );
    }

    for( SPARSELIZARD_DIELECTRIC dielectric : m_dielectrics )
    {
        ( *m_equations ) +=
                sl::integral( dielectric.regionID, sl::grad( sl::tf( m_v ) ) * ( *m_epsilon )
                                                           * sl::grad( sl::dof( m_v ) ) );
    }
}

void SPARSELIZARD_SOLVER::writeResults( FEM_DESCRIPTOR* aDescriptor )
{
    int  wholedomain = sl::selectall();
    //static int k = 0;
    //m_j.write( wholedomain, to_string(k++) +".pos", MAX_ORDER );
    sl::fieldorder( m_v ).write( wholedomain, "fieldOrder.pos" );        //TODO: remove

    for( FEM_RESULT* result : aDescriptor->GetResults() )
    {
        if( result == nullptr )
        {
            m_reporter->Report( "A FEM_RESULT is NULL", RPT_SEVERITY_ERROR );
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
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computePotentialDC( regionA );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::VOLTAGE:
            {
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR regionB = findConductor( resultValue->GetPortB() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computeVoltageDC( regionA, regionB );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::CURRENT:
            {
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }

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
                    resultValue->m_value = computeCurrentDC( regionA );
                }
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::RESISTANCE:
            {
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR regionB = findConductor( resultValue->GetPortB() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computeResistanceDC( regionA, regionB );
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
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR regionB = findConductor( resultValue->GetPortB() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }

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
                    resultValue->m_value = computePowerDC( regionA, regionB );
                    resultValue->m_valid = true;
                }
                break;
            }
            case FEM_VALUE_TYPE::CAPACITANCE:
            {
                SPARSELIZARD_CONDUCTOR regionA = findConductor( resultValue->GetPortA() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR regionB = findConductor( resultValue->GetPortB() );
                if( regionA.regionID == 0 )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }

                resultValue->m_value = computeCapacitanceDC( regionA, regionB );
                resultValue->m_valid = true;
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
}

SPARSELIZARD_CONDUCTOR SPARSELIZARD_SOLVER::findConductor( FEM_PORT* aPort )
{
    for( SPARSELIZARD_CONDUCTOR cond : m_conductors )
    {
        if( cond.femPort == aPort )
        {
            return cond;
        }
    }
    SPARSELIZARD_CONDUCTOR dummy;
    dummy.regionID = 0;
    return dummy;
}

bool SPARSELIZARD_SOLVER::Run_DC( FEM_DESCRIPTOR* aDescriptor )
{
    const double copperResistivity = 1.72e-8 * 1000; //1e-30;//

    std::vector<int> conductorRegions;
    std::vector<int> dielectricRegions;


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

    for( FEM_PORT* portA : aDescriptor->GetPorts() )
    {
        if( portA->GetItem() == nullptr )
        {
            m_reporter->Report( "A FEM_PORT is NULL", RPT_SEVERITY_ERROR );
            continue;
        }

        std::list<int>::iterator it;
        it = std::find( netlist.begin(), netlist.end(), portA->GetItem()->GetNetCode() );
        if( it == netlist.end() )
        {
            netlist.push_back( portA->GetItem()->GetNetCode() );
        }

        switch( portA->GetItem()->Type() )
        {
        case PCB_PAD_T:
        {
            portA->m_simulationID =
                    mesher.AddPadRegion( static_cast<const PAD*>( portA->GetItem() ) );

            SPARSELIZARD_CONDUCTOR conductor;
            conductor.rho = copperResistivity;
            conductor.regionID = portA->m_simulationID;
            conductor.netCode = static_cast<const PAD*>( portA->GetItem() )->GetNetCode();
            port prim;
            port dual;
            conductor.primalPort = prim;
            conductor.dualPort = dual;
            conductor.femPort = portA;
            m_conductors.push_back( conductor );
            conductorRegions.push_back( portA->m_simulationID );
            break;
        }
        default: break;
        }
    }

    m_reporter->Report( "Number of ports in simulation: " + to_string( m_conductors.size() ),
                        RPT_SEVERITY_INFO );

    // Add copper zones / tracks

    for( int netID : netlist )
    {
        SPARSELIZARD_CONDUCTOR conductor;
        conductor.rho = copperResistivity;
        conductor.regionID = mesher.AddNetRegion( netID );
        conductor.netCode = netID;
        m_conductors.push_back( conductor );
        conductorRegions.push_back( conductor.regionID );
    }

    if( aDescriptor->m_requiresDielectric )
    {
        for( int region : mesher.AddDielectricRegions() )
        {
            SPARSELIZARD_DIELECTRIC dielectric;
            dielectric.epsilonr = 1;
            dielectric.regionID = region;
            m_dielectrics.push_back( dielectric );
            dielectricRegions.push_back( dielectric.regionID );
        }
    }

    m_reporter->Report( "Number of conducting regions in simulation: "
                                + to_string( m_conductors.size() ),
                        RPT_SEVERITY_INFO );
    m_reporter->Report( "Number of dielectric regions in simulation: "
                                + to_string( m_dielectrics.size() ),
                        RPT_SEVERITY_INFO );

    m_reporter->Report( "SPARSELIZARD: Loading mesh...", RPT_SEVERITY_ACTION );


#ifdef USE_GMSH
    switch( aDescriptor->m_dim )
    {
    case FEM_SIMULATION_DIMENSION::SIMUL2D5: mesher.Load25DMesh(); break;
    case FEM_SIMULATION_DIMENSION::SIMUL3D: mesher.Load3DMesh(); break;
    default:
        m_reporter->Report( "Simulation dimension is not supported", RPT_SEVERITY_ERROR );
        return false;
    }

    mesh mymesh( "gmsh:api", SPARSELIZARD_VERBOSITY );
    mesher.Finalize();
#else
    std::vector<shape>  shapes;
    mesher.Get2DShapes( shapes, PCB_LAYER_ID::F_Cu, true );
    mesh mymesh;
    mymesh.split( 2 );
    mymesh.load( shapes );
#endif

    m_reporter->Report( "SPARSELIZARD: mesh loaded.", RPT_SEVERITY_INFO );

    mymesh.write( "mymesh.msh", SPARSELIZARD_VERBOSITY );

    // Create the electric potential field v
    m_v = field( "h1" );
    formulation eq;
    m_equations = &eq;
    // parameters
    parameter rho;     // resistivity
    parameter epsilon; // permittivity

    m_rho = &rho;
    m_epsilon = &epsilon;
    m_reporter->Report( "Setting up simulation parameters...", RPT_SEVERITY_ACTION );
    if( !setParameters( aDescriptor ) )
        return false;
    m_reporter->Report( "Setting up simulation ports...", RPT_SEVERITY_ACTION );
    setConstraints( aDescriptor );
    m_reporter->Report( "Setting up simulation equations...", RPT_SEVERITY_ACTION );
    setEquations();
    // Expression for the electric field E [V/m] and current density j [A/m^2]:
    m_E = -sl::grad( m_v );                 // electric field
    m_j = m_E / ( *m_rho );                 // current density
    m_p = sl::doubledotproduct( m_j, m_E ); // power density


    int  nb_refinements = 0;
    bool need_adapt = false;

    m_v.setorder( sl::norm( sl::grad( m_v ) ), MIN_ORDER, MAX_ORDER + 1 );


    m_reporter->Report( "SPARSELIZARD: Simulating...", RPT_SEVERITY_ACTION );

    // Simple p-adaptivity loop:
    for( int i = 0; i <= MAX_ORDER - MIN_ORDER; i++ )
    {
        //m_equations->solve( "cholesky" );
        m_equations->solve();
        need_adapt = sl::adapt( SPARSELIZARD_VERBOSITY );
        if( !need_adapt )
        {
            break; // No adaptation was needed
        };
    }

    m_reporter->Report( "SPARSELIZARD: End of simulation.", RPT_SEVERITY_INFO );

    m_v.write( sl::selectunion( conductorRegions ), std::string( "potential.pos" ),
               MAX_ORDER ); //TODO: remove
    m_j.write( sl::selectunion( conductorRegions ), "currentDensity.pos" ); //TODO: remove
    writeResults( aDescriptor );
    m_reporter->Report( "Finished", RPT_SEVERITY_INFO );
    return true;
}
