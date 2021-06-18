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
#include <vector>

#define USE_GMSH

#define MIN_ORDER 2
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


std::vector<int> SPARSELIZARD_SOLVER::getAllRegionsWithNetcode( int aNetCode,
                                                                int aIgnoredPort = -1 )
{
    std::vector<int> regions;

    for( SPARSELIZARD_CONDUCTOR* conductor : m_conductors )
    {
        if( ( conductor->netCode == aNetCode ) && ( conductor->regionID != aIgnoredPort ) )
        {
            regions.push_back( conductor->regionID );
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
    double QA = aConA.dualPort.getvalue();
    double V = computeVoltageDC( aConA, aConB );
    std::cout << "The computed capacitance is " << QA / V << std::endl;
    return QA / V;
}

void SPARSELIZARD_SOLVER::setVoltageDC( SPARSELIZARD_CONDUCTOR aCon, double aV )
{
    port V, I;
    V = aCon.primalPort;
    I = aCon.dualPort;
    V.setvalue( aV );
    m_v.setport( aCon.boundaryID, V, I );
    //m_v.setconstraint( aCon.regionID, aV );
    ( *m_equations ) += V - aV;
}

void SPARSELIZARD_SOLVER::setCurrentDC( SPARSELIZARD_CONDUCTOR aCon, double aI )
{
    port V, I;
    V = aCon.primalPort;
    I = aCon.dualPort;
    m_v.setport( aCon.boundaryID, V, I );
    ( *m_equations ) += I - aI;
}

void SPARSELIZARD_SOLVER::setChargeDC( SPARSELIZARD_CONDUCTOR aCon, double aQ )
{
    int dielectrics = sl::selectunion( m_dielectricRegions );
    aCon.boundaryID = sl::selectintersection( { aCon.regionID, dielectrics } );
    port V, Q;
    V = aCon.primalPort;
    Q = aCon.dualPort;
    m_v.setport( aCon.boundaryID, V, Q );
    ( *m_equations ) += Q - aQ;
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
            SPARSELIZARD_CONDUCTOR* region = findConductor( portA );
            if( region == nullptr )
            {
                m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                continue;
            }

            switch( portA->m_constraint.m_type )
            {
            case FEM_PORT_CONSTRAINT_TYPE::VOLTAGE:
                setVoltageDC( *region, portA->m_constraint.m_value );
                break;
            case FEM_PORT_CONSTRAINT_TYPE::CURRENT:
            {
                setCurrentDC( *region, portA->m_constraint.m_value );
                break;
            }
            case FEM_PORT_CONSTRAINT_TYPE::CHARGE:
            {
                setChargeDC( *region, portA->m_constraint.m_value );
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
    for( SPARSELIZARD_DIELECTRIC* dielectric : m_dielectrics )
    {
        m_v.setorder( dielectric->regionID, 1 );

        if( aDescriptor->m_simulateElectricField )
            ( *m_epsilon ) | dielectric->regionID = dielectric->epsilonr * m_constants.epsilon0();


        if( aDescriptor->m_simulateMagneticField )
        {
            m_A.setorder( dielectric->regionID, 0 );
            ( *m_mu ) | dielectric->regionID = 1;
        }
    }

    for( SPARSELIZARD_CONDUCTOR* conductor : m_conductors )
    {
        m_v.setorder( conductor->regionID, 1 );
        switch( aDescriptor->m_dim )
        {
        case FEM_SIMULATION_DIMENSION::SIMUL2D5:
            ( *m_rho ) | conductor->regionID = conductor->rho / ( 35e-3 );
            break;
        case FEM_SIMULATION_DIMENSION::SIMUL3D:
            ( *m_rho ) | conductor->regionID = conductor->rho;
            break;
        default:
            m_reporter->Report( "Simulation dimension is not supported", RPT_SEVERITY_ERROR );
            return false;
        }

        if( aDescriptor->m_simulateMagneticField )
        {
            ( *m_mu ) | conductor->regionID = 1;
            m_A.setorder( conductor->regionID, 0 );
        }
    }
    return true;
}

void SPARSELIZARD_SOLVER::setEquations( FEM_DESCRIPTOR* aDescriptor )
{
    if( aDescriptor->m_simulateMagneticField )
    {
        m_A.setgauge( sl::selectall() );
    }

    switch( aDescriptor->m_simulationType )
    {
    case FEM_SIMULATION_TYPE::DC:
        if( aDescriptor->m_simulateConductor )
        {
            std::cout << "Setting conductors..." << std::endl;
            for( SPARSELIZARD_CONDUCTOR* conductor : m_conductors )
            {
                // Divergence of current density ( E / rho ) = 0
                ( *m_equations ) +=
                        sl::integral( conductor->regionID, sl::grad( sl::tf( m_v ) ) / ( *m_rho )
                                                                   * sl::grad( sl::dof( m_v ) ) );


                if( aDescriptor->m_simulateMagneticField )
                {
                    // Ampere's law
                    ( *m_equations ) += sl::integral(
                            conductor->regionID,
                            1 / ( *m_mu ) * sl::curl( sl::dof( m_A ) ) * sl::curl( sl::tf( m_A ) )
                                    + 1 / ( *m_rho ) * sl::grad( sl::dof( m_v ) ) * sl::tf( m_A ) );
                    // No magnetic flux can cross the domain
                    m_A.setconstraint( m_boundary, 0 );
                }
            }
        }
        std::cout << "Setting dielectrics..." << std::endl;

        for( SPARSELIZARD_DIELECTRIC* dielectric : m_dielectrics )
        {
            if( aDescriptor->m_simulateElectricField )
            {
                // Gauss' Law
                ( *m_equations ) += sl::integral( dielectric->regionID,
                                                  -( *m_epsilon ) * sl::grad( sl::dof( m_v ) )
                                                          * sl::grad( sl::tf( m_v ) ) );
            }
            if( aDescriptor->m_simulateMagneticField )
            {
                // Ampere's law
                ( *m_equations ) += sl::integral( dielectric->regionID,
                                                  1 / ( *m_mu ) * sl::curl( sl::dof( m_A ) )
                                                          * sl::curl( sl::tf( m_A ) ) );
            }
        }

        std::cout << "Equations OK" << std::endl;

        // Expression for the electric field E [V/m] and current density j [A/m^2]:
        m_E = -sl::grad( m_v );                 // electric field
        m_j = m_E / ( *m_rho );                 // current density
        m_p = sl::doubledotproduct( m_j, m_E ); // power density

        break;
    case FEM_SIMULATION_TYPE::AC:
        /*
        if( aDescriptor->m_simulateConductor )
        {
            for( SPARSELIZARD_CONDUCTOR* conductor : m_conductors )
            {
                //( *m_equations ) += sl::integral(conductor->regionID, 1/( *m_mu )* sl::curl(sl::dof(m_A)) * sl::curl(sl::tf(m_A)) ); // Gauss's law for magnetism
            }
        }

        if( aDescriptor->m_simulateDielectric )
        {
            for( SPARSELIZARD_DIELECTRIC* dielectric : m_dielectrics )
            {
                //( *m_equations ) += sl::integral(dielectric->regionID, 1/( *m_mu )* sl::curl(sl::dof(m_A)) * sl::curl(sl::tf(m_A)) ); // Gauss's law for magnetism
            }
        }*/
        break;

    default:
        m_reporter->Report( "Simulation type is not supported (while setting equations)",
                            RPT_SEVERITY_ERROR );
        return;
    }
}

void SPARSELIZARD_SOLVER::writeResults( FEM_DESCRIPTOR* aDescriptor )
{
    int  wholedomain = sl::selectall();
    //sl::fieldorder( m_v ).write( wholedomain, "fieldOrder.pos" );        //TODO: remove

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
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computePotentialDC( *regionA );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::VOLTAGE:
            {
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR* regionB = findConductor( resultValue->GetPortB() );
                if( regionB == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computeVoltageDC( *regionA, *regionB );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::CURRENT:
            {
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
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
                    resultValue->m_value = computeCurrentDC( *regionA );
                }
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::RESISTANCE:
            {
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR* regionB = findConductor( resultValue->GetPortB() );
                if( regionB == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                resultValue->m_value = computeResistanceDC( *regionA, *regionB );
                resultValue->m_valid = true;
                break;
            }
            case FEM_VALUE_TYPE::DISSIPATED_POWER:
            {
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }

                if( regionA->femPort->m_type == FEM_PORT_TYPE::PASSIVE )
                {
                    resultValue->m_value =
                            ( sl::on( regionA->regionID, m_p ) ).integrate( regionA->regionID, 4 );
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
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR* regionB = findConductor( resultValue->GetPortB() );
                if( regionB == nullptr )
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
                    resultValue->m_value = computePowerDC( *regionA, *regionB );
                    resultValue->m_valid = true;
                }
                break;
            }
            case FEM_VALUE_TYPE::CAPACITANCE:
            {
                SPARSELIZARD_CONDUCTOR* regionA = findConductor( resultValue->GetPortA() );
                if( regionA == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }
                SPARSELIZARD_CONDUCTOR* regionB = findConductor( resultValue->GetPortB() );
                if( regionB == nullptr )
                {
                    m_reporter->Report( "Could not match conductor with port", RPT_SEVERITY_ERROR );
                    continue;
                }

                resultValue->m_value = computeCapacitanceDC( *regionA, *regionB );
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
                m_j.write( sl::selectunion( m_conductorRegions ),
                           std::string( resultView->m_filename.GetFullName() ), MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::VOLTAGE:
                m_v.write( sl::selectunion( m_conductorRegions ),
                           std::string( resultView->m_filename.GetFullName() ), MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::POWER:
                m_p.write( sl::selectunion( m_conductorRegions ),
                           std::string( resultView->m_filename.GetFullName() ), MAX_ORDER );
                resultView->m_valid = true;
                break;
            case FEM_VIEW_TYPE::ELECTRIC_FIELD:
                if( aDescriptor->m_simulateElectricField )
                {
                    m_v.write( sl::selectunion( m_dielectricRegions ),
                               std::string( resultView->m_filename.GetFullName() ), MAX_ORDER );
                    resultView->m_valid = true;
                }
                else
                {
                    m_reporter->Report( "You asked for a electric field plot. But electric field "
                                        "was not simulated ( m_simulateElectricField ).",
                                        RPT_SEVERITY_ERROR );
                }
                break;
            case FEM_VIEW_TYPE::MAGNETIC_FIELD:
                if( aDescriptor->m_simulateMagneticField )
                {
                    ( sl::curl( m_A ) )
                            .write( sl::selectunion( m_dielectricRegions ),
                                    std::string( resultView->m_filename.GetFullName() ),
                                    MAX_ORDER );
                    m_A.write( wholedomain, "a.pos", MAX_ORDER );
                    resultView->m_valid = true;
                }
                else
                {
                    m_reporter->Report( "You asked for a magnetic field plot. But magnetic field "
                                        "was not simulated ( m_simulateMagneticField ).",
                                        RPT_SEVERITY_ERROR );
                }
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

SPARSELIZARD_CONDUCTOR* SPARSELIZARD_SOLVER::findConductor( FEM_PORT* aPort )
{
    for( SPARSELIZARD_CONDUCTOR* cond : m_conductors )
    {
        if( cond->femPort == aPort )
        {
            return cond;
        }
    }
    return nullptr;
}

void SPARSELIZARD_SOLVER::SetRegions( FEM_DESCRIPTOR* aDescriptor, GMSH_MESHER* aMesher )
{
    std::list<int> netlist;

    for( FEM_PORT* portA : aDescriptor->GetPorts() )
    {
        if( portA->GetItem() == nullptr )
        {
            m_reporter->Report( "A FEM_PORT is NULL", RPT_SEVERITY_ERROR );
            continue;
        }

        switch( portA->GetItem()->Type() )
        {
        case PCB_NETINFO_T:
        {
            SPARSELIZARD_CONDUCTOR* conductor = new SPARSELIZARD_CONDUCTOR();
            conductor->type = PCB_NETINFO_T;
            conductor->rho = m_constants.rho_Cu();
            conductor->netCode = static_cast<const NETINFO_ITEM*>( portA->GetItem() )->GetNetCode();
            conductor->regionID = aMesher->AddNetRegion( conductor->netCode );
            m_conductors.push_back( conductor );
            m_conductorRegions.push_back( conductor->regionID );

            port prim;
            port dual;
            conductor->primalPort = prim;
            conductor->dualPort = dual;
            conductor->femPort = portA;
            break;
        }
        case PCB_PAD_T:
        {
            SPARSELIZARD_CONDUCTOR* conductor = new SPARSELIZARD_CONDUCTOR();
            conductor->type = PCB_PAD_T;
            conductor->rho = m_constants.rho_Cu();
            conductor->regionID =
                    aMesher->AddPadRegion( static_cast<const PAD*>( portA->GetItem() ) );
            conductor->netCode = static_cast<const PAD*>( portA->GetItem() )->GetNetCode();
            port prim;
            port dual;
            conductor->primalPort = prim;
            conductor->dualPort = dual;
            conductor->femPort = portA;
            m_conductors.push_back( conductor );
            m_conductorRegions.push_back( conductor->regionID );

            std::list<int>::iterator it;
            it = std::find( netlist.begin(), netlist.end(), conductor->netCode );
            if( it == netlist.end() )
            {
                netlist.push_back( conductor->netCode );
            }
            std::cout << "Conductor ( PAD ) id: " << conductor->regionID << std::endl;
            break;
        }
        default:
            m_reporter->Report( "Cannot set port on this KICAD_T Type:"
                                        + to_string( portA->GetItem()->Type() ),
                                RPT_SEVERITY_ERROR );
            break;
        }
    }

    m_reporter->Report( "Number of ports in simulation: " + to_string( m_conductors.size() ),
                        RPT_SEVERITY_INFO );
    m_reporter->Report( "Number of ports in simulation: "
                                + to_string( aDescriptor->GetPorts().size() ),
                        RPT_SEVERITY_INFO );

    // Add copper zones / tracks

    for( int netID : netlist )
    {
        SPARSELIZARD_CONDUCTOR* conductor = new SPARSELIZARD_CONDUCTOR();
        conductor->type = PCB_NETINFO_T; // We just assume it is like a PCB_NETINFO
        conductor->rho = m_constants.rho_Cu();
        conductor->regionID = aMesher->AddNetRegion( netID );
        conductor->netCode = netID;
        conductor->femPort = nullptr;
        m_conductors.push_back( conductor );
        m_conductorRegions.push_back( conductor->regionID );
    }

    if( aDescriptor->m_requiresDielectric )
    {
        SPARSELIZARD_DIELECTRIC* dielectric;
        for( GMSH_MESHER_LAYER region : aMesher->AddDielectricRegions() )
        {
            dielectric = new SPARSELIZARD_DIELECTRIC();
            dielectric->epsilonr = region.permittivity;
            dielectric->regionID = region.regionID;
            dielectric->lossTangent = region.lossTangent;
            m_dielectrics.push_back( dielectric );
            m_dielectricRegions.push_back( dielectric->regionID );
        }
    }

    if( aDescriptor->m_requiresAir )
    {
        SPARSELIZARD_DIELECTRIC* air;
        air = new SPARSELIZARD_DIELECTRIC();
        air->epsilonr = 1;
        air->regionID = aMesher->AddAirRegion();
        air->lossTangent = 0;
        m_dielectrics.push_back( air );
        m_dielectricRegions.push_back( air->regionID );

        m_boundary = aMesher->AddDomainBoundaryRegion();
        std::cout << "boundary will be on region " << m_boundary << std::endl;
    }

    m_reporter->Report( "Number of conducting regions in simulation: "
                                + to_string( m_conductors.size() ),
                        RPT_SEVERITY_INFO );
    m_reporter->Report( "Number of dielectric regions in simulation: "
                                + to_string( m_dielectrics.size() ),
                        RPT_SEVERITY_INFO );
}

void SPARSELIZARD_SOLVER::SetBoundaries()
{
    for( SPARSELIZARD_CONDUCTOR* cond : m_conductors )
    {
        if( cond->femPort != nullptr )
        {
            switch( cond->type )
            {
            case PCB_NETINFO_T:
            {
                std::vector<int> everythingElse;
                everythingElse.insert( everythingElse.end(), m_conductorRegions.begin(),
                                       m_conductorRegions.end() );
                everythingElse.insert( everythingElse.end(), m_dielectricRegions.begin(),
                                       m_dielectricRegions.end() );
                std::remove( everythingElse.begin(), everythingElse.end(), cond->regionID );
                everythingElse.pop_back();
                cond->boundaryID = sl::selectintersection(
                        { cond->regionID, sl::selectunion( everythingElse ) } );
                break;
            }
            case PCB_PAD_T:
            {
                std::vector<int> everythingElse;
                everythingElse.insert( everythingElse.end(), m_conductorRegions.begin(),
                                       m_conductorRegions.end() );
                everythingElse.insert( everythingElse.end(), m_dielectricRegions.begin(),
                                       m_dielectricRegions.end() );
                std::remove( everythingElse.begin(), everythingElse.end(), cond->regionID );
                everythingElse.pop_back();
                cond->boundaryID = sl::selectintersection(
                        { cond->regionID, sl::selectunion( everythingElse ) } );
                break;
            }
            default:
                m_reporter->Report( "Cannot find boundary on this KICAD_T Type:"
                                            + to_string( cond->type ),
                                    RPT_SEVERITY_ERROR );
                break;
            }
        }
    }
}

void SPARSELIZARD_SOLVER::simulate()
{
    int  nb_refinements = 0;
    bool need_adapt = false;
    m_equations->solve();
    //m_equations->solve( "cholesky" ); // Don't use cholesky with sparselizard ports !

    //m_v.setorder( sl::norm( sl::grad( m_v ) ), MIN_ORDER, MAX_ORDER + 1 );
    //m_H.setorder( sl::norm( sl::grad( m_H ) ), MIN_ORDER, MAX_ORDER + 1 );
    // Simple p-adaptivity loop:
    /*
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
    */
}

bool SPARSELIZARD_SOLVER::Run_DC( FEM_DESCRIPTOR* aDescriptor )
{
    m_reporter->Report( "Sparselizard version: " + sl::getversionname(), RPT_SEVERITY_INFO );
    if( !m_constants.SetUnitScaling( EDA_UNITS::MILLIMETRES ) )
    {
        m_reporter->Report( "Cannot get constants in this unit system", RPT_SEVERITY_ERROR );
        return false;
    }

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
    SetRegions( aDescriptor, &mesher );
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

    //m_boundary = aMesher->AddDomainBoundaryRegion();
    //std::cout << "Boundary will be on region: "<< m_boundary << std::endl;m_boundary

    std::cout << "Boundary will be on region: " << m_boundary << std::endl;

    // Remove empty regions ( Can happen when adding nets )
    for( SPARSELIZARD_CONDUCTOR* cond : m_conductors )
    {
        if( !sl::isdefined( cond->regionID ) )
        {
            m_reporter->Report( "SPARSELIZARD: Removing undefined region "
                                        + to_string( cond->regionID ),
                                RPT_SEVERITY_ERROR );
            std::remove( m_conductors.begin(), m_conductors.end(), cond );
            m_conductors.pop_back();
            std::remove( m_conductorRegions.begin(), m_conductorRegions.end(), cond->regionID );
            m_conductorRegions.pop_back();
        }
    }


    mymesh.write( "mymesh.msh", SPARSELIZARD_VERBOSITY );

    SetBoundaries();
    for( auto cond : m_conductors )
    {
        std::cout << "conductor: " << cond->regionID << std::endl;
        std::cout << "conductor boundary: " << cond->boundaryID << std::endl;
    }

    std::cout << " spantree OK " << std::endl;

    // Create the electric potential field v
    switch( aDescriptor->m_simulationType )
    {
    case FEM_SIMULATION_TYPE::DC:
        m_v = field( "h1" );
        if( aDescriptor->m_simulateMagneticField )
        {
            spanningtree spantree( { m_boundary } );
            m_A = field( "hcurl", spantree );
            spantree.write( "spantree.pos" );
        }
        break;
    case FEM_SIMULATION_TYPE::AC:
        m_v = field( "h1", { 2, 3 } );
        m_A = field( "hcurl", { 2, 3 } );
        break;
    default:
        m_reporter->Report( "Simulation type is not supported (while setting fields",
                            RPT_SEVERITY_ERROR );
        return false;
    }

    formulation eq;
    m_equations = &eq;

    // parameters
    parameter rho;     // resistivity
    parameter epsilon; // permittivity
    parameter mu;      // permeability
    m_rho = &rho;
    m_epsilon = &epsilon;
    m_mu = &mu;
    m_reporter->Report( "Setting up simulation parameters...", RPT_SEVERITY_ACTION );

    if( !setParameters( aDescriptor ) )
        return false;

    m_reporter->Report( "Setting up simulation ports...", RPT_SEVERITY_ACTION );
    setConstraints( aDescriptor );

    m_reporter->Report( "Setting up simulation equations...", RPT_SEVERITY_ACTION );
    setEquations( aDescriptor );

    m_reporter->Report( "SPARSELIZARD: Simulating...", RPT_SEVERITY_ACTION );
    simulate();

    m_reporter->Report( "SPARSELIZARD: End of simulation.", RPT_SEVERITY_INFO );
    ( -sl::grad( m_v ) )
            .write( sl::selectunion( m_dielectricRegions ), std::string( "Efield.pos" ),
                    MAX_ORDER ); //TODO: remove
    m_v.write( sl::selectunion( m_conductorRegions ), std::string( "potential.pos" ),
               MAX_ORDER ); //TODO: remove
    m_j.write( sl::selectunion( m_conductorRegions ), "currentDensity.pos",
               MAX_ORDER ); //TODO: remove

    if( aDescriptor->m_requiresAir )
    {
        expression( 1 ).write( m_boundary, "boundary.pos", 1 );
    }
    writeResults( aDescriptor );
    m_reporter->Report( "Finished", RPT_SEVERITY_INFO );
    return true;
}
