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

#include <iostream>
#include <string>

#include <wx/init.h>
#include <pgm_base.h>

#include <pcbnew_utils/board_file_utils.h>

#include <pcbnew/fem/sparselizard/sparselizard_mesher.h>

#include <sparselizard/sparselizard.h>

#include <pcbnew/board.h>

using namespace sl;


void runFEMProto( const BOARD* aBoard, std::string aNetname )
{
    SPARSELIZARD_MESHER mesher( aBoard );

    NETINFO_ITEM* gndNetinfo = aBoard->FindNet( aNetname );
    if( gndNetinfo == nullptr )
    {
        std::cerr << "Net not found: '" << aNetname << "'" << std::endl;
        return;
    }

    int regionId = mesher.AddNetRegion( gndNetinfo->GetNetCode() );
    std::cerr << "Net '" << aNetname << "' will be on region: " << regionId << std::endl;

    std::vector<shape> shapes;

    // offset front CU so we can see both sides :D
    /*mesher.Get2DShapes( shapes, PCB_LAYER_ID::F_Cu );
    for(auto& shape : shapes)
    {
        shape.shift(0, 0, 10.);
    }*/

    mesher.Get2DShapes( shapes, PCB_LAYER_ID::B_Cu, true );

    mesh mymesh( shapes );

    mymesh.write( "mymesh.msh" );
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

    runFEMProto( board.get(), "GND" );

    Pgm().Destroy();
    wxUninitialize();

    return 0;
}
