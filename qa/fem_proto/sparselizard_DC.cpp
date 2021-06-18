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


#include <unit_test_utils/unit_test_utils.h>
// Code under test

#include <iostream>
#include <string>

#include <wx/init.h>
#include <pgm_base.h>

#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew/fem/sparselizard/sparselizard_solver.h>
#include <pcb_edit_frame.h>
#include <pcbnew/board.h>
#include <pcb_shape.h>
#include <io_mgr.h>
#include <board_commit.h>
#include "../pcbnew_utils/include/pcbnew_utils/board_file_utils.h"

class TEST_SPARSELIZARD_DC
{
public:
    TEST_SPARSELIZARD_DC(){};
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SparselizardDC, class TEST_SPARSELIZARD_DC )

/**
 * Function testCurrentConservation
 *
 * Simulates a 0V voltage source, and current sinks.
 * The current from the voltage source should equal all current flowing through the current sinks.
 * 
 */

bool testCurrentConservation( double x, double y, double d, double I, double nbSources,
                              double max_error, FEM_SIMULATION_DIMENSION aDim )
{
    BOARD*     board = new BOARD();
    FOOTPRINT* fp = new FOOTPRINT( board );

    // Init board stackup
    board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &board->GetDesignSettings() );

    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, board );
    descriptor->m_simulationType = FEM_SIMULATION_TYPE::DC;
    descriptor->m_dim = aDim;

    PAD* pad1 = new PAD( fp );
    pad1->SetShape( PAD_SHAPE::RECT );
    pad1->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, x * 1000 ) );
    pad1->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, y * 1000 ) );
    pad1->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad1->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad1->SetNetCode( 1, true );
    pad1->SetAttribute( PAD_ATTRIB::SMD );
    pad1->SetLayer( PCB_LAYER_ID::F_Cu );
    pad1->SetLayerSet( pad1->SMDMask() );
    pad1->SetDrillSizeX( 0 );
    pad1->SetDrillSizeY( 0 );

    for( int i = 0; i < nbSources; i++ )
    {
        PAD* pad2 = new PAD( fp );
        pad2->SetShape( PAD_SHAPE::RECT );
        pad2->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, x * 1000 ) );
        pad2->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, y * 1000 ) );
        pad2->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, ( i + 1 ) * d * 1000 ) );
        pad2->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
        pad2->SetNetCode( 1, true );
        pad2->SetAttribute( PAD_ATTRIB::SMD );
        pad2->SetLayer( PCB_LAYER_ID::F_Cu );
        pad2->SetLayerSet( pad1->SMDMask() );
        pad2->SetDrillSizeX( 0 );
        pad2->SetDrillSizeY( 0 );


        FEM_PORT*            port2 = new FEM_PORT( pad2 );
        FEM_PORT_CONSTRAINT* constraint2 = new FEM_PORT_CONSTRAINT();

        constraint2->m_type = FEM_PORT_CONSTRAINT_TYPE::CURRENT;
        constraint2->m_value = 1; // 1 A
        port2->m_type = FEM_PORT_TYPE::SOURCE;
        port2->m_constraint = *constraint2;
        descriptor->AddPort( port2 );
    }

    TRACK* track = new TRACK( board );
    track->SetWidth( From_User_Unit( EDA_UNITS::MILLIMETRES, 1.5 * x * 1000 ) );
    track->SetStart( wxPoint( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ),
                              From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) ) );
    track->SetEnd( wxPoint( From_User_Unit( EDA_UNITS::MILLIMETRES, nbSources * I * d * 1000 ),
                            From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) ) );
    track->SetNetCode( 1, true );
    board->Add( fp );
    board->Add( track );

    descriptor->m_reporter = new NULL_REPORTER(); // Don't report

    FEM_PORT*            port1 = new FEM_PORT( pad1 );
    FEM_PORT_CONSTRAINT* constraint1 = new FEM_PORT_CONSTRAINT();


    constraint1->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
    constraint1->m_value = 0; // 0V
    port1->m_type = FEM_PORT_TYPE::SOURCE;
    port1->m_constraint = *constraint1;
    descriptor->AddPort( port1 );

    FEM_RESULT_VALUE* r_current = new FEM_RESULT_VALUE( FEM_VALUE_TYPE::CURRENT, port1, nullptr );

    if( !( r_current )->IsInitialized() )
        std::cerr << "Could not initialize current result. " << std::endl;

    if( !descriptor->AddResult( r_current ) )
        std::cerr << "Could not add current result to descriptor " << std::endl;

    descriptor->Run();

    if( r_current->m_valid )
    {
        if( I == 0 )
        {
            if( r_current->GetResult() != 0 )
            {
                std::cout << "test failed: I=" << -r_current->GetResult() << " instead of "
                          << nbSources * I << std::endl;
                return false;
            }
            return true;
        }
        if( abs( -r_current->GetResult() - nbSources * I ) / ( nbSources * I ) > max_error )
        {
            std::cout << "test failed: I=" << -r_current->GetResult() << " instead of "
                      << nbSources * I << std::endl;
            return false;
        }
        return true;
    }
    return false;
}


/**
 * Function testTrackResistance
 *
 * Simulates a track resistance using sparselizard.
 * If the result does not match the theoretical return false. Otherwise, return true.
 * 
 * if overshoot is true, then two extra tracks are added beyond the measurements ports. The result should not change.
 * 
 * @param rho = copper resisitivity
 * @param L = track length, in meters
 * @param h = track height, in meters
 * @param w = track width, in meters
 * @param max_error = maximum relative error before failure
 * @param overshoot = True: add two other tracks.
 */

bool simulTrackResistance( double rho, double L, double h, double w, double max_error,
                           FEM_SIMULATION_DIMENSION aDim, bool overshoot = false )
{
    double correctValue = rho * L / ( h * w );

    BOARD*     board = new BOARD();
    FOOTPRINT* fp = new FOOTPRINT( board );

    // Init board stackup
    board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &board->GetDesignSettings() );


    BOARD_STACKUP& stackup = board->GetDesignSettings().GetStackupDescriptor();
    
    for( BOARD_STACKUP_ITEM* layer : stackup.GetList() )
    {
        if( layer->GetBrdLayerId() == PCB_LAYER_ID::F_Cu )
        {
            layer->SetThickness( From_User_Unit( EDA_UNITS::MILLIMETRES, h * 1000 ) );
            break;
        }
    }
    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, board );
    descriptor->m_dim = aDim;
    descriptor->m_simulationType = FEM_SIMULATION_TYPE::DC;

    PAD* pad1 = new PAD( fp );
    pad1->SetShape( PAD_SHAPE::RECT );
    pad1->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, 1 ) );
    pad1->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, 2 * w * 1000 ) );
    pad1->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, -( L * 1000 ) / 2 - 0.5 ) );
    pad1->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad1->SetNetCode( 1, true );
    pad1->SetAttribute( PAD_ATTRIB::SMD );
    pad1->SetLayer( PCB_LAYER_ID::F_Cu );
    pad1->SetLayerSet( pad1->SMDMask() );
    pad1->SetDrillSizeX( 0 );
    pad1->SetDrillSizeY( 0 );


    PAD* pad2 = new PAD( fp );
    pad2->SetShape( PAD_SHAPE::RECT );
    pad2->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, 1 ) );
    pad2->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, 2 * w * 1000 ) );
    pad2->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, ( L * 1000 ) / 2 + 0.5 ) );
    pad2->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad2->SetNetCode( 1, true );
    pad2->SetAttribute( PAD_ATTRIB::SMD );
    pad2->SetLayer( PCB_LAYER_ID::F_Cu );
    pad2->SetLayerSet( pad1->SMDMask() );
    pad2->SetDrillSizeX( 0 );
    pad2->SetDrillSizeY( 0 );


    PAD* pad3 = new PAD( fp );
    pad3->SetShape( PAD_SHAPE::RECT );
    pad3->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, L * 1000 ) );
    pad3->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, w * 1000 ) );
    pad3->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad3->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad3->SetNetCode( 1, true );
    pad3->SetAttribute( PAD_ATTRIB::SMD );
    pad3->SetLayer( PCB_LAYER_ID::F_Cu );
    pad3->SetLayerSet( pad1->SMDMask() );
    pad3->SetDrillSizeX( 0 );
    pad3->SetDrillSizeY( 0 );

    fp->Add( pad1 );
    fp->Add( pad2 );
    fp->Add( pad3 );

    PAD* pad4;
    PAD* pad5;
    if( overshoot )
    {
        pad4 = new PAD( fp );
        pad4->SetShape( PAD_SHAPE::RECT );
        pad4->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, L * 1000 ) );
        pad4->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, w * 1000 ) );
        pad4->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, -( L * 2 * 1000 ) / 2 - 1 ) );
        pad4->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
        pad4->SetNetCode( 1, true );
        pad4->SetAttribute( PAD_ATTRIB::SMD );
        pad4->SetLayer( PCB_LAYER_ID::F_Cu );
        pad4->SetLayerSet( pad1->SMDMask() );
        pad4->SetDrillSizeX( 0 );
        pad4->SetDrillSizeY( 0 );

        FEM_PORT* port4 = new FEM_PORT( pad4 );
        port4->m_type = FEM_PORT_TYPE::PASSIVE;
        descriptor->AddPort( port4 );

        pad5 = new PAD( fp );
        pad5->SetShape( PAD_SHAPE::RECT );
        pad5->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, L * 1000 ) );
        pad5->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, w * 1000 ) );
        pad5->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, ( L * 2 * 1000 ) / 2 + 1 ) );
        pad5->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
        pad5->SetNetCode( 1, true );
        pad5->SetAttribute( PAD_ATTRIB::SMD );
        pad5->SetLayer( PCB_LAYER_ID::F_Cu );
        pad5->SetLayerSet( pad1->SMDMask() );
        pad5->SetDrillSizeX( 0 );
        pad5->SetDrillSizeY( 0 );

        FEM_PORT* port5 = new FEM_PORT( pad5 );
        port5->m_type = FEM_PORT_TYPE::PASSIVE;
        descriptor->AddPort( port5 );


        fp->Add( pad4 );
        fp->Add( pad5 );
    }

    board->Add( fp );

    descriptor->m_reporter = new STDOUT_REPORTER(); // Don't report

    FEM_PORT*            port1 = new FEM_PORT( pad1 );
    FEM_PORT_CONSTRAINT* constraint1 = new FEM_PORT_CONSTRAINT();

    constraint1->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
    constraint1->m_value = 1; // 1 V
    port1->m_type = FEM_PORT_TYPE::SOURCE;
    port1->m_constraint = *constraint1;
    descriptor->AddPort( port1 );

    FEM_PORT*            port2 = new FEM_PORT( pad2 );
    FEM_PORT_CONSTRAINT* constraint2 = new FEM_PORT_CONSTRAINT();

    constraint2->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
    constraint2->m_value = 0; // 0 V
    port2->m_type = FEM_PORT_TYPE::SOURCE;
    port2->m_constraint = *constraint2;
    descriptor->AddPort( port2 );

    FEM_RESULT_VALUE* r_resistance =
            new FEM_RESULT_VALUE( FEM_VALUE_TYPE::RESISTANCE, port1, port2 );

    if( !( r_resistance )->IsInitialized() )
        std::cerr << "Could not initialize resistance result. " << std::endl;

    if( !descriptor->AddResult( r_resistance ) )
        std::cerr << "Could not add resistance result to descriptor " << std::endl;

    descriptor->Run();

    if( r_resistance->m_valid )
    {
        if( abs( r_resistance->GetResult() - correctValue ) / correctValue > max_error )
        {
            std::cout << "test failed: R=" << r_resistance->GetResult() << " instead of "
                      << correctValue << std::endl;
            return false;
        }
        return true;
    }
    return false;
}



bool simulPlaneCapacitance( double x, double y, double epsilonr, double d, double aMaxError )
{
    double epsilon0 = 8.8541878128e-12;
    double correctValue = epsilon0 * epsilonr * x * y / d;

    BOARD*     board = new BOARD();
    FOOTPRINT* fp1 = new FOOTPRINT( board );
    FOOTPRINT* fp2 = new FOOTPRINT( board );


    // Init board stackup
    board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &board->GetDesignSettings() );

    BOARD_STACKUP& stackup = board->GetDesignSettings().GetStackupDescriptor();

    bool nextIsDielectric = false;
    int  i = 0;
    for( BOARD_STACKUP_ITEM* layer : stackup.GetList() )
    {
        if( nextIsDielectric )
        {
            std::cout << " Dielectric: " << i << std::endl;
            layer->SetThickness( From_User_Unit( EDA_UNITS::MILLIMETRES, d * 1000 ) );
            layer->SetEpsilonR( epsilonr );
            nextIsDielectric = false;
        }
        if( layer->GetBrdLayerId() == PCB_LAYER_ID::F_Cu )
        {
            std::cout << " F_Cu: " << i << std::endl;
            layer->SetThickness( From_User_Unit( EDA_UNITS::MILLIMETRES, 1 ) );
            nextIsDielectric = true;
        }
        if( layer->GetBrdLayerId() == PCB_LAYER_ID::B_Cu )
        {
            std::cout << " B_Cu: " << i << std::endl;
            layer->SetThickness( From_User_Unit( EDA_UNITS::MILLIMETRES, 1 ) );
        }
        i++;
    }

    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, board );
    descriptor->m_reporter = new STDOUT_REPORTER();
    descriptor->m_dim = FEM_SIMULATION_DIMENSION::SIMUL3D;
    descriptor->m_simulationType = FEM_SIMULATION_TYPE::DC;
    descriptor->m_requiresDielectric = true;
    descriptor->m_requiresAir = false;
    descriptor->m_simulateConductor = false;
    descriptor->m_simulateElectricField = true;

    PAD* pad1 = new PAD( fp1 );
    pad1->SetShape( PAD_SHAPE::RECT );
    pad1->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, x * 1000 ) );
    pad1->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, y * 1000 ) );
    pad1->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad1->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad1->SetNetCode( 1, true );
    pad1->SetAttribute( PAD_ATTRIB::SMD );
    pad1->SetLayer( PCB_LAYER_ID::F_Cu );
    pad1->SetLayerSet( pad1->SMDMask() );
    pad1->SetDrillSizeX( 0 );
    pad1->SetDrillSizeY( 0 );

    PAD* pad2 = new PAD( fp2 );
    pad2->SetShape( PAD_SHAPE::RECT );
    pad2->SetSizeX( From_User_Unit( EDA_UNITS::MILLIMETRES, x * 1000 ) );
    pad2->SetSizeY( From_User_Unit( EDA_UNITS::MILLIMETRES, y * 1000 ) );
    pad2->SetX( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad2->SetY( From_User_Unit( EDA_UNITS::MILLIMETRES, 0 ) );
    pad2->SetNetCode( 1, true );
    pad2->SetAttribute( PAD_ATTRIB::SMD );
    pad2->SetLayer( PCB_LAYER_ID::F_Cu );
    pad2->SetLayerSet( pad2->SMDMask() );
    pad2->SetDrillSizeX( 0 );
    pad2->SetDrillSizeY( 0 );

    const wxPoint center = wxPoint( 0, 0 );
    pad2->Flip( center, false );


    board->Add( fp1 );
    board->Add( fp2 );

    PCB_SHAPE* rect = new PCB_SHAPE;
    rect->SetShape( PCB_SHAPE_TYPE::RECT );
    rect->SetFilled( false );
    rect->SetStartX( From_User_Unit( EDA_UNITS::MILLIMETRES, -x / 2 * 1000 ) );
    rect->SetStartY( From_User_Unit( EDA_UNITS::MILLIMETRES, -y / 2 * 1000 ) );
    rect->SetEndX( From_User_Unit( EDA_UNITS::MILLIMETRES, x / 2 * 1000 ) );
    rect->SetEndY( From_User_Unit( EDA_UNITS::MILLIMETRES, y / 2 * 1000 ) );
    rect->SetWidth( From_User_Unit( EDA_UNITS::MILLIMETRES, 0.05 ) );
    rect->SetLayer( PCB_LAYER_ID::Edge_Cuts );
    board->Add( rect );

    FEM_PORT*            port1 = new FEM_PORT( pad1 );
    FEM_PORT_CONSTRAINT* constraint1 = new FEM_PORT_CONSTRAINT();

    constraint1->m_type = FEM_PORT_CONSTRAINT_TYPE::CHARGE;
    constraint1->m_value = 1e-9; // C
    port1->m_type = FEM_PORT_TYPE::SOURCE;
    port1->m_constraint = *constraint1;
    descriptor->AddPort( port1 );

    FEM_PORT*            port2 = new FEM_PORT( pad2 );
    FEM_PORT_CONSTRAINT* constraint2 = new FEM_PORT_CONSTRAINT();

    constraint2->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
    constraint2->m_value = 0; // 0 V
    port2->m_type = FEM_PORT_TYPE::SOURCE;
    port2->m_constraint = *constraint2;
    descriptor->AddPort( port2 );

    FEM_RESULT_VALUE* r_capacitance =
            new FEM_RESULT_VALUE( FEM_VALUE_TYPE::CAPACITANCE, port1, port2 );

    if( !( r_capacitance )->IsInitialized() )
        std::cerr << "Could not initialize capacitance result. " << std::endl;

    if( !descriptor->AddResult( r_capacitance ) )
        std::cerr << "Could not add capacitance result to descriptor " << std::endl;

    descriptor->Run();

    if( r_capacitance->m_valid )
    {
        if( abs( r_capacitance->GetResult() - correctValue ) / correctValue > aMaxError )
        {
            std::cout << "test failed: C=" << r_capacitance->GetResult() << " instead of "
                      << correctValue << std::endl;
            return false;
        }
        return true;
    }
    return false;
}


void trackResistanceTest( FEM_SIMULATION_DIMENSION aDim )
{
    double rho = 1.72e-8;      // copper resistivity
    double L = 200e-3;         // track length
    double h = 35e-6;          // track height
    double w = 30e-3;          // track width
    double max_error = 1e-8;
    BOOST_CHECK_EQUAL( simulTrackResistance( rho, L, h, w, max_error, aDim ), true );
    BOOST_CHECK_EQUAL( simulTrackResistance( rho, L, h, w, max_error, aDim, true ), true );

    rho = 1.72e-8; // copper resistivity
    L = 2e-3;      // track length
    h = 35e-6;     // track height
    w = 30e-3;     // track width
    BOOST_CHECK_EQUAL( simulTrackResistance( rho, L, h, w, max_error, aDim ), true );

    rho = 1.72e-8; // copper resistivity
    L = 200e-3;    // track length
    h = 17e-6;     // track height
    w = 30e-3;     // track width
    BOOST_CHECK_EQUAL( simulTrackResistance( rho, L, h, w, max_error, aDim ), true );

    rho = 1.72e-8; // copper resistivity
    L = 200e-6;    // track length
    h = 35e-6;     // track height
    w = 35e-6;     // track width
    BOOST_CHECK_EQUAL( simulTrackResistance( rho, L, h, w, max_error, aDim, true ), true );
}

void currentConservationTest( FEM_SIMULATION_DIMENSION aDim )
{
    double I = 1;
    double nbSink = 2;
    double x = 5e-3;
    double y = 5e-3;
    double d = 15e-3;
    double maxError = 1e-9;
    BOOST_CHECK_EQUAL( testCurrentConservation( x, y, d, I, nbSink, maxError, aDim ), true );

    I = 0;
    nbSink = 2;
    x = 5e-3;
    y = 5e-3;
    d = 15e-3;
    BOOST_CHECK_EQUAL( testCurrentConservation( x, y, d, I, nbSink, maxError, aDim ), true );

    I = 1;
    nbSink = 4;
    x = 5e-3;
    y = 5e-3;
    d = 15e-3;
    BOOST_CHECK_EQUAL( testCurrentConservation( x, y, d, I, nbSink, maxError, aDim ), true );
}

void planeCapacitanceTest()
{
    double epsilonr = 1;
    double d = 1.5e-3;
    double x = 5e-3;
    double y = 10e-3;
    double maxError = 1e-9;
    BOOST_CHECK_EQUAL( simulPlaneCapacitance( x, y, epsilonr, d, maxError ), true );

    epsilonr = 4.4;
    d = 1.5e-3;
    x = 5e-3;
    y = 10e-3;
    BOOST_CHECK_EQUAL( simulPlaneCapacitance( x, y, epsilonr, d, maxError ), true );

    epsilonr = 1;
    d = 1.5e-3;
    x = 10e-3;
    y = 10e-3;
    BOOST_CHECK_EQUAL( simulPlaneCapacitance( x, y, epsilonr, d, maxError ), true );

    epsilonr = 1;
    d = 10e-3;
    x = 10e-3;
    y = 10e-3;
    BOOST_CHECK_EQUAL( simulPlaneCapacitance( x, y, epsilonr, d, maxError ), true );
}

BOOST_AUTO_TEST_CASE( TestTrackResistance )
{
    trackResistanceTest( FEM_SIMULATION_DIMENSION::SIMUL2D5 );
    trackResistanceTest( FEM_SIMULATION_DIMENSION::SIMUL3D );
}

BOOST_AUTO_TEST_CASE( TestPlaneCapacitance )
{
    planeCapacitanceTest();
}

BOOST_AUTO_TEST_CASE( TestCurrentConservation )
{
    //currentConservationTest( FEM_SIMULATION_DIMENSION::SIMUL2D5 );
    //currentConservationTest( FEM_SIMULATION_DIMENSION::SIMUL3D );
}

BOOST_AUTO_TEST_SUITE_END()
