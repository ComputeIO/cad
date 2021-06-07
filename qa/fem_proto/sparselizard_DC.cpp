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

#include <iostream>
#include <string>

#include <wx/init.h>
#include <pgm_base.h>

#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew/fem/sparselizard/sparselizard_solver.h>

#include <pcbnew/board.h>


bool testTrackResistance( double rho, double L, double h, double w, double max_error )
{
    double correct_value = rho * L / ( h * w );

    BOARD*     board = new BOARD();
    FOOTPRINT* fp = new FOOTPRINT( board );

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

    TRACK* track = new TRACK( board );
    track->SetWidth( From_User_Unit( EDA_UNITS::MILLIMETRES, 1 ) );
    track->SetStart( wxPoint( From_User_Unit( EDA_UNITS::MILLIMETRES, -120 ),
                              From_User_Unit( EDA_UNITS::MILLIMETRES, 10 ) ) );
    track->SetEnd( wxPoint( From_User_Unit( EDA_UNITS::MILLIMETRES, -120 ),
                            From_User_Unit( EDA_UNITS::MILLIMETRES, -10 ) ) );
    track->SetNetCode( 1, true );

    board->Add( fp );
    board->Add( track ); // Should be removed

    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, board );
    descriptor->m_reporter = new NULL_REPORTER(); // Don't report

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


    FEM_PORT* port3 = new FEM_PORT( pad3 );
    port3->m_type = FEM_PORT_TYPE::PASSIVE;
    descriptor->AddPort( port3 );

    FEM_RESULT_VALUE* r_resistance =
            new FEM_RESULT_VALUE( FEM_VALUE_TYPE::RESISTANCE, port1, port2 );

    if( !( r_resistance )->IsInitialized() )
        std::cerr << "Could not initialize resistance result. " << std::endl;

    if( !descriptor->AddResult( r_resistance ) )
        std::cerr << "Could not add resistance result to descriptor " << std::endl;

    descriptor->Run();

    if( r_resistance->m_valid )
    {
        if( abs( r_resistance->GetResult() - correct_value ) / correct_value < max_error )
        {
            std::cout << "test passed" << std::endl;
            return true;
        }
        else
        {
            std::cout << "test failed: R=" << r_resistance->GetResult() << " instead of "
                      << correct_value << std::endl;
            return false;
        }
    }
}

int main( int argc, char** argv )
{
    wxInitialize( argc, argv );
    Pgm().InitPgm();


    double rho = 1.72e-8;      // copper resistivity
    double L = 200e-3;         // track length
    double h = 35e-6;          // track height
    double w = 30e-3;          // track width
    double max_error = 0.0001; // 0.01%

    if( !testTrackResistance( rho, L, h, w, max_error ) )
    {
        std::cout << "TEST FAILED." << std::endl;
    }

    rho = 1.72e-8; // copper resistivity
    L = 2e-3;      // track length
    h = 35e-6;     // track height
    w = 30e-3;     // track width

    if( !testTrackResistance( rho, L, h, w, max_error ) )
    {
        std::cout << "TEST FAILED." << std::endl;
    }

    rho = 1.72e-8; // copper resistivity
    L = 200e-3;    // track length
    h = 17e-6;     // track height
    w = 30e-3;     // track width

    if( !testTrackResistance( rho, L, h, w, max_error ) )
    {
        std::cout << "TEST FAILED." << std::endl;
    }

    rho = 1.72e-8; // copper resistivity
    L = 200e-3;    // track length
    h = 35e-6;     // track height
    w = 0.1e-3;    // track width

    double correct_value = rho * L / ( h * w );

    if( !testTrackResistance( rho, L, h, w, max_error ) )
    {
        std::cout << "TEST FAILED." << std::endl;
    }

    Pgm().Destroy();
    wxUninitialize();

    return 0;
}
