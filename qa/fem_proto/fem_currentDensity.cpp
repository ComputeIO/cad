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

#include <pcbnew/fem/sparselizard/sparselizard_mesher.h>
#include <pcbnew/fem/sparselizard/sparselizard_solver.h>

#include <sparselizard/sparselizard.h>

#include <pcbnew/board.h>

using namespace sl;


void runFEMCurrentDensity( const BOARD* aBoard )
{
    std::cout << "Current density from J1-pad1 to J2-pad1" << std::endl;
    wxString netName;

    FEM_PORT        port1;
    FEM_PORT        port2;
    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, aBoard );

    const FOOTPRINT* footprint1 = aBoard->FindFootprintByReference( "J1" );

    if( footprint1 != nullptr )
    {
        const PAD* pad = footprint1->FindPadByName( "1" );
        if( pad != nullptr )
        {
            netName = pad->GetNetname();

            FEM_PORT_CONSTRAINT* constraint1 = new FEM_PORT_CONSTRAINT();
            constraint1->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
            constraint1->m_value = 0.001; // 1 mV

            port1.m_type = FEM_PORT_TYPE::SOURCE;
            port1.m_constraint = constraint1;
            port1.m_item = pad;
        }
        else
        {
            std::cerr << "  Could not find J1 - Pad 1 " << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "  Could not find J1 - Pad 1 " << std::endl;
        return;
    }

    const FOOTPRINT* footprint2 = aBoard->FindFootprintByReference( "J2" );

    if( footprint2 != nullptr )
    {
        const PAD* pad = footprint2->FindPadByName( "1" );
        if( pad != nullptr )
        {
            if( netName != pad->GetNetname() )
            {
                std::cerr << "  J1 - Pad 1 and J1 - Pad 2 are not connected." << std::endl;
            }

            FEM_PORT_CONSTRAINT* constraint2 = new FEM_PORT_CONSTRAINT();
            constraint2->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
            constraint2->m_value = 0; // 1 mV

            port2.m_type = FEM_PORT_TYPE::SOURCE;
            port2.m_constraint = constraint2;
            port2.m_item = pad;
        }
        else
        {
            std::cerr << "  Could not find J1 - Pad 1 " << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "  Could not find J1 - Pad 1 " << std::endl;
        return;
    }
    std::cout << "Board loaded" << std::endl;

    descriptor->AddPort( &port1 );
    descriptor->AddPort( &port2 );

    Run_DC_CurrentDensity( descriptor );
}


int main( int argc, char** argv )
{
    wxInitialize( argc, argv );
    Pgm().InitPgm();

    if( argc < 2 )
    {
        printf( "usage: %s <project-file/board-file>\n", argv[0] );
        Pgm().Destroy();
        wxUninitialize();
        return -1;
    }

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( std::string( argv[1] ) );

    runFEMCurrentDensity( board.get() );

    Pgm().Destroy();
    wxUninitialize();

    return 0;
}
