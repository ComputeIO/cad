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
    vector<shape> shapes;

    PCB_LAYER_ID klayer = PCB_LAYER_ID::F_Cu;
    for( size_t i = 0; i < board->GetAreaCount(); i++ )
    {
        ZONE* area = board->GetArea( i );
        if( area->IsFilled() && area->IsOnLayer( klayer ) )
        {
            area->CacheTriangulation( klayer );

            const SHAPE_POLY_SET& polyset = area->GetFilledPolysList( klayer );
            for( size_t k = 0; k < polyset.TriangulatedPolyCount(); k++ )
            {
                const SHAPE_POLY_SET::TRIANGULATED_POLYGON* triangulated =
                        polyset.TriangulatedPolygon( k );

                std::cout << "* Found area on " << area->GetLayerName() << " with "
                          << triangulated->GetTriangleCount() << " triangles" << std::endl;
                for( size_t t = 0; t < triangulated->GetTriangleCount(); t++ )
                {
                    VECTOR2<int> a, b, c;
                    triangulated->GetTriangle( t, a, b, c );

                    std::vector<double> coords = { a.x / IU_PER_MM, a.y / IU_PER_MM, 0,
                                                   b.x / IU_PER_MM, b.y / IU_PER_MM, 0,
                                                   c.x / IU_PER_MM, c.y / IU_PER_MM, 0 };

                    /*); i < coords.size(); ++i){
                        std::ostringstream doubleStr;
                        doubleStr << coords[i];
                        tempStr += doubleStr.str() + ", ";
                    }
                    std::cout << tempStr << std::endl;*/

                    shape mytriangle( "triangle", area->GetNetCode(), coords, { 2, 2, 2 } );
                    shapes.push_back( mytriangle );
                }
            }
        }
    }

    /*int   quadranglephysicalregion = 1, trianglephysicalregion = 2, unionphysicalregion = 3;
    shape line1( "line", -1, { -1, -1, 0, 1, -1, 0 }, 10 );
    shape arc2( "arc", -1, { 1, -1, 0, 1, 1, 0, 0, 0, 0 }, 12 );
    shape line3( "line", -1, { 1, 1, 0, -1, 1, 0 }, 10 );
    shape line4( "line", -1, { -1, 1, 0, -1, -1, 0 }, 12 );
    shape line5( "line", -1, { 1, -1, 0, 3, -1, 0 }, 12 );
    shape arc6( "arc", -1, { 3, -1, 0, 1, 1, 0, 1.6, -0.4, 0 }, 12 );
    shape myquadrangle( "quadrangle", quadranglephysicalregion, { line1, arc2, line3, line4 } );*/
    //shape mytriangle("triangle",  trianglephysicalregion , {line5 ,arc6, arc2});
    //shape myunion("union", unionphysicalregion, {line1, arc2, line3 ,line4 });

    // Add here all regions needed in the finite element simulation.
    mesh mymesh( shapes );

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
