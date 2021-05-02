#include <iostream>
#include <string>

#include <wx/init.h>
#include <pgm_base.h>

#include <pcbnew_utils/board_file_utils.h>

#include <sparselizard/sparselizard.h>

#include <pcbnew/board.h>

using namespace sl;


void runFEMProto( const BOARD* board )
{
    int   quadranglephysicalregion = 1, trianglephysicalregion = 2, unionphysicalregion = 3;
    shape line1( "line", -1, { -1, -1, 0, 1, -1, 0 }, 10 );
    shape arc2( "arc", -1, { 1, -1, 0, 1, 1, 0, 0, 0, 0 }, 12 );
    shape line3( "line", -1, { 1, 1, 0, -1, 1, 0 }, 10 );
    shape line4( "line", -1, { -1, 1, 0, -1, -1, 0 }, 12 );
    shape line5( "line", -1, { 1, -1, 0, 3, -1, 0 }, 12 );
    shape arc6( "arc", -1, { 3, -1, 0, 1, 1, 0, 1.6, -0.4, 0 }, 12 );
    shape myquadrangle( "quadrangle", quadranglephysicalregion, { line1, arc2, line3, line4 } );
    //shape mytriangle("triangle",  trianglephysicalregion , {line5 ,arc6, arc2});
    //shape myunion("union", unionphysicalregion, {line1, arc2, line3 ,line4 });

    // Add here all regions needed in the finite element simulation.
    mesh mymesh( { myquadrangle } );

    // You can write the mesh at any time during the geometry
    // construction to easily debug and validate every line of code.
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

    runFEMProto( board.get() );

    Pgm().Destroy();
    wxUninitialize();

    return 0;
}
