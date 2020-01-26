#include "class_board.h"
#include "3d_render_ogl_legacy/clayer_triangles.h"

#include <geometry/geometry_utils.h>
#include "3d_render_raytracing/accelerators/ccontainer2d.h"
#include "3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "3d_render_raytracing/shapes2D/croundsegment2d.h"
#include "3d_render_raytracing/shapes2D/cpolygon4pts2d.h"
#include "3d_render_raytracing/shapes2D/ctriangle2d.h"
#include "3d_render_raytracing/shapes2D/cring2d.h"

#include "geom3d.h"

#define SIZE_OF_CIRCLE_TEXTURE 1024

// TODO determine if this needs to follow logic from CINFO3D_VISU (prob not, we
// want to maintain consistent scale)
const double biu_to_3d_units = 1.0;

#define COPPER_THICKNESS KiROUND( 0.035 * IU_PER_MM )   // for 35 um
const float copperThickness3DU = COPPER_THICKNESS * biu_to_3d_units;

unsigned int GetNrSegmentsCircle( float aDiameter3DU )
{
    wxASSERT( aDiameter3DU > 0.0f );

    return GetNrSegmentsCircle( (int)( aDiameter3DU / biu_to_3d_units ) );
}

double GetCircleCorrectionFactor( int aNrSides )
{
    wxASSERT( aNrSides >= 3 );

    return GetCircletoPolyCorrectionFactor( aNrSides );
}

bool Is3DLayerEnabled( PCB_LAYER_ID aLayer )
{
    // TODO
    return true;
}

unsigned int GetStatsNrVias( BOARD* board, float* med_via_hole_diameter )
{
    unsigned int nr_vias = 0;
    med_via_hole_diameter = 0;

    for( const TRACK* track = board->m_Track; track; track = track->Next() )
    {
        if( !Is3DLayerEnabled( track->GetLayer() ) ) // Skip non enabled layers
            continue;

        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast< const VIA*>( track );
            nr_vias++;
            *med_via_hole_diameter += via->GetDrillValue() * biu_to_3d_units;
        }
    }

    *med_via_hole_diameter /= (float)nr_vias;

    return nr_vias;
}

void get_layer_z_pos ( PCB_LAYER_ID aLayerID,
                       const BOARD* board,
                       float &aOutZtop,
                       float &aOutZbot )
{
    unsigned int layer = aLayerID;

    const float epoxyThickness3DU = board->GetDesignSettings().GetBoardThickness() *
                          biu_to_3d_units;

    const unsigned int copperLayersCount = board->GetCopperLayerCount();

    aOutZbot = epoxyThickness3DU / 2.0f -
               (epoxyThickness3DU * layer / (copperLayersCount - 1) );

    if( layer < (copperLayersCount / 2) )
        aOutZtop = aOutZbot + copperThickness3DU;
    else
        aOutZtop = aOutZbot - copperThickness3DU;

    if( aOutZtop < aOutZbot )
    {
        float tmpFloat = aOutZbot;
        aOutZbot = aOutZtop;
        aOutZtop = tmpFloat;
    }
}

//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void generate_ring_contour( const SFVEC2F &aCenter,
                            float aInnerRadius,
                            float aOuterRadius,
                            unsigned int aNr_sides_per_circle,
                            std::vector< SFVEC2F > &aInnerContourResult,
                            std::vector< SFVEC2F > &aOuterContourResult,
                            bool aInvertOrder )
{
    aInnerContourResult.clear();
    aInnerContourResult.reserve( aNr_sides_per_circle + 2 );

    aOuterContourResult.clear();
    aOuterContourResult.reserve( aNr_sides_per_circle + 2 );

    const int delta = 3600 / aNr_sides_per_circle;

    for( int ii = 0; ii < 3600; ii += delta )
    {
        float angle = (float)( aInvertOrder ? ( 3600 - ii ) : ii )
                            * 2.0f * glm::pi<float>() / 3600.0f;
        const SFVEC2F rotatedDir = SFVEC2F( cos( angle ), sin( angle ) );

        aInnerContourResult.push_back( SFVEC2F( aCenter.x + rotatedDir.x * aInnerRadius,
                                                aCenter.y + rotatedDir.y * aInnerRadius ) );

        aOuterContourResult.push_back( SFVEC2F( aCenter.x + rotatedDir.x * aOuterRadius,
                                                aCenter.y + rotatedDir.y * aOuterRadius ) );
    }

    aInnerContourResult.push_back( aInnerContourResult[0] );
    aOuterContourResult.push_back( aOuterContourResult[0] );

    wxASSERT( aInnerContourResult.size() == aOuterContourResult.size() );
}

//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void generate_cylinder( const SFVEC2F &aCenter,
                        float aInnerRadius,
                        float aOuterRadius,
                        float aZtop,
                        float aZbot,
                        unsigned int aNr_sides_per_circle,
                        CLAYER_TRIANGLES *aDstLayer )
{
    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generate_ring_contour( aCenter,
                           aInnerRadius,
                           aOuterRadius,
                           aNr_sides_per_circle,
                           innerContour,
                           outerContour,
                           false );

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F &vi0 = innerContour[i + 0];
        const SFVEC2F &vi1 = innerContour[i + 1];
        const SFVEC2F &vo0 = outerContour[i + 0];
        const SFVEC2F &vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }

    aDstLayer->AddToMiddleContourns( outerContour, aZbot, aZtop, true );
    aDstLayer->AddToMiddleContourns( innerContour, aZbot, aZtop, false );
}

CLAYER_TRIANGLES* generate_3D_Vias( BOARD* board )
{
    float med_via_hole_diameter = 0;
    const unsigned int nr_vias = GetStatsNrVias( board, &med_via_hole_diameter );

    if( nr_vias )
    {
        const float thickness = copperThickness3DU;

        const unsigned int reserve_nr_triangles_estimation =
                GetNrSegmentsCircle( med_via_hole_diameter ) *
                8 *
                nr_vias;

        CLAYER_TRIANGLES *layerTriangleVIA = new CLAYER_TRIANGLES( reserve_nr_triangles_estimation );

        // Insert plated vertical holes inside the board
        // /////////////////////////////////////////////////////////////////////////

        // Insert vias holes (vertical cylinders)
        for( const TRACK* track = board->m_Track;
             track;
             track = track->Next() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast<const VIA*>(track);

                const float  holediameter = via->GetDrillValue() * biu_to_3d_units;
                const int    nrSegments = GetNrSegmentsCircle( via->GetDrillValue() );
                const double correctionFactor = GetCircleCorrectionFactor( nrSegments );
                const float  hole_inner_radius = ( holediameter / 2.0f ) * correctionFactor;

                const SFVEC2F via_center(  via->GetStart().x * biu_to_3d_units,
                                          -via->GetStart().y * biu_to_3d_units );

                PCB_LAYER_ID top_layer, bottom_layer;
                via->LayerPair( &top_layer, &bottom_layer );

                float ztop, zbot, dummy;

                get_layer_z_pos( top_layer,    board, ztop,  dummy );
                get_layer_z_pos( bottom_layer, board, dummy, zbot );

                wxASSERT( zbot < ztop );

                generate_cylinder( via_center,
                                   hole_inner_radius,
                                   hole_inner_radius + thickness,
                                   ztop,
                                   zbot,
                                   nrSegments,
                                   layerTriangleVIA );
            }
        }

        return layerTriangleVIA;
    }

    return new CLAYER_TRIANGLES( 0 );
}

COBJECT2D* createNewTrack( const TRACK* aTrack,
                           int aClearanceValue )
{
    SFVEC2F start3DU(  aTrack->GetStart().x * biu_to_3d_units,
                      -aTrack->GetStart().y * biu_to_3d_units ); // y coord is inverted

    switch( aTrack->Type() )
    {
    case PCB_VIA_T:
    {
        const float radius = ( ( aTrack->GetWidth() / 2 ) + aClearanceValue ) * biu_to_3d_units;

        return new CFILLEDCIRCLE2D( start3DU, radius, *aTrack );
    }
        break;

    default:
    {
        wxASSERT( aTrack->Type() == PCB_TRACE_T );

        SFVEC2F end3DU (  aTrack->GetEnd().x * biu_to_3d_units,
                         -aTrack->GetEnd().y * biu_to_3d_units );

        // Cannot add segments that have the same start and end point
        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            const float radius = ((aTrack->GetWidth() / 2) + aClearanceValue) * biu_to_3d_units;

            return new CFILLEDCIRCLE2D( start3DU, radius, *aTrack );
        }
        else
        {
            const float width = (aTrack->GetWidth() + 2 * aClearanceValue ) * biu_to_3d_units;

            return new CROUNDSEGMENT2D( start3DU, end3DU, width, *aTrack );
        }
    }
        break;
    }

    return NULL;
}

//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_triangle_top_bot( CLAYER_TRIANGLES *aDst,
                           const SFVEC2F &v0,
                           const SFVEC2F &v1,
                           const SFVEC2F &v2,
                           float top,
                           float bot )
{
    aDst->m_layer_bot_triangles->AddTriangle( SFVEC3F( v0.x, v0.y, bot ),
                                              SFVEC3F( v1.x, v1.y, bot ),
                                              SFVEC3F( v2.x, v2.y, bot ) );

    aDst->m_layer_top_triangles->AddTriangle( SFVEC3F( v2.x, v2.y, top ),
                                              SFVEC3F( v1.x, v1.y, top ),
                                              SFVEC3F( v0.x, v0.y, top ) );
}

//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_object_to_triangle_layer( const CFILLEDCIRCLE2D * aFilledCircle,
                                   CLAYER_TRIANGLES *aDstLayer,
                                   float aZtop,
                                   float aZbot )
{
    const SFVEC2F &center = aFilledCircle->GetCenter();
    const float radius = aFilledCircle->GetRadius() *
                         2.0f; // Double because the render triangle

    // This is a small adjustment to the circle texture
    const float texture_factor = (8.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;
    const float f = (sqrtf(2.0f) / 2.0f) * radius * texture_factor;

    // Top and Bot segments ends are just triangle semi-circles, so need to add
    // it in duplicated
    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x,
                                                               center.y - f, aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x,
                                                               center.y + f, aZtop ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x,
                                                               center.y - f, aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x,
                                                               center.y + f, aZbot ) );
}


//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_object_to_triangle_layer( const CPOLYGON4PTS2D * aPoly,
                                   CLAYER_TRIANGLES *aDstLayer,
                                   float aZtop,
                                   float aZbot )
{
    const SFVEC2F &v0 = aPoly->GetV0();
    const SFVEC2F &v1 = aPoly->GetV1();
    const SFVEC2F &v2 = aPoly->GetV2();
    const SFVEC2F &v3 = aPoly->GetV3();

    add_triangle_top_bot( aDstLayer, v0, v2, v1, aZtop, aZbot );
    add_triangle_top_bot( aDstLayer, v2, v0, v3, aZtop, aZbot );
}

//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_object_to_triangle_layer( const CRING2D * aRing,
                                   CLAYER_TRIANGLES *aDstLayer,
                                   float aZtop,
                                   float aZbot )
{
    const SFVEC2F &center = aRing->GetCenter();
    const float inner = aRing->GetInnerRadius();
    const float outer = aRing->GetOuterRadius();

    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generate_ring_contour( center,
                           inner,
                           outer,
                           GetNrSegmentsCircle( outer * 2.0f ),
                           innerContour,
                           outerContour,
                           false );

    // This will add the top and bot quads that will form the approximated ring

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F &vi0 = innerContour[i + 0];
        const SFVEC2F &vi1 = innerContour[i + 1];
        const SFVEC2F &vo0 = outerContour[i + 0];
        const SFVEC2F &vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }
}


//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_object_to_triangle_layer( const CTRIANGLE2D * aTri,
                                   CLAYER_TRIANGLES *aDstLayer,
                                   float aZtop,
                                   float aZbot )
{
    const SFVEC2F &v1 = aTri->GetP1();
    const SFVEC2F &v2 = aTri->GetP2();
    const SFVEC2F &v3 = aTri->GetP3();

    add_triangle_top_bot( aDstLayer, v1, v2, v3, aZtop, aZbot );
}


//TODO deduplicate with c3d_render_createscene_ogl_legacy.cpp
void add_object_to_triangle_layer( const CROUNDSEGMENT2D * aSeg,
                                   CLAYER_TRIANGLES *aDstLayer,
                                   float aZtop,
                                   float aZbot )
{
    const SFVEC2F leftStart   = aSeg->GetLeftStar();
    const SFVEC2F leftEnd     = aSeg->GetLeftEnd();
    const SFVEC2F leftDir     = aSeg->GetLeftDir();

    const SFVEC2F rightStart  = aSeg->GetRightStar();
    const SFVEC2F rightEnd    = aSeg->GetRightEnd();
    const SFVEC2F rightDir    = aSeg->GetRightDir();
    const float   radius      = aSeg->GetRadius();

    const SFVEC2F start       = aSeg->GetStart();
    const SFVEC2F end         = aSeg->GetEnd();

    const float texture_factor = (12.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;
    const float texture_factorF= ( 6.0f / (float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;

    const float radius_of_the_square = sqrtf( aSeg->GetRadiusSquared() * 2.0f );
    const float radius_triangle_factor = (radius_of_the_square - radius) / radius;

    const SFVEC2F factorS = SFVEC2F( -rightDir.y * radius * radius_triangle_factor,
                                      rightDir.x * radius * radius_triangle_factor );

    const SFVEC2F factorE = SFVEC2F( -leftDir.y  * radius * radius_triangle_factor,
                                      leftDir.x  * radius * radius_triangle_factor );

    // Top end segment triangles (semi-circles)
    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZtop ),
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZtop ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZtop ),
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZtop ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    // Bot end segment triangles (semi-circles)
    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZbot ),
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZbot ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZbot ),
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZbot ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    // Segment top and bot planes
    aDstLayer->m_layer_top_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZtop ),
                SFVEC3F( rightStart.x, rightStart.y, aZtop ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZtop ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZtop ) );

    aDstLayer->m_layer_bot_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZbot ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZbot ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZbot ),
                SFVEC3F( rightStart.x, rightStart.y, aZbot ) );
}

CLAYER_TRIANGLES* generate_3D_layer( const BOARD* board, const PCB_LAYER_ID layer_id ) {
    // ii->second here is from m_settings.GetMapLayers()
    //
    // which is of type
    // typedef std::map< PCB_LAYER_ID, CBVHCONTAINER2D *> MAP_CONTAINER_2D;
    //
    // so ii->second is of type CBVHCONTAINER2D
    //
    // which is held in m_settings.m_layers_container2D[layer_id]
    //
    // which is created for layer_id in 3d-viewer/3d_canvas/create_layer_items.cpp
    CBVHCONTAINER2D *container2d = new CBVHCONTAINER2D;

    unsigned int nTracks = board->m_Track.GetCount();
    std::vector< const TRACK *> trackList;
    trackList.reserve( nTracks );

    //if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
    //    (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    //{
    SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
    //    m_layers_poly[layer_id] = layerPoly;
    //}

    for( const TRACK* track = board->m_Track; track; track = track->Next() )
    {
        // Note: a TRACK holds normal segment tracks and
        // also vias circles (that have also drill values)
        if( track->IsOnLayer( layer_id ) )
          trackList.push_back( track ); //TODO do we need trackList??

        container2d->Add( createNewTrack( track, 0.0f ) );

        // Add the track contour
        int nrSegments = GetNrSegmentsCircle( track->GetWidth() );

        track->TransformShapeWithClearanceToPolygon(
                    *layerPoly,
                    0,
                    nrSegments,
                    GetCircleCorrectionFactor( nrSegments ) );
    }

    //const CBVHCONTAINER2D *container2d = static_cast<const CBVHCONTAINER2D *>(ii->second);
    const LIST_OBJECT2D &listObject2d = container2d->GetList();

    if( listObject2d.size() == 0 ) {
        return nullptr;
    }

    float layer_z_bot = 0.0f;
    float layer_z_top = 0.0f;

    get_layer_z_pos( layer_id, board, layer_z_top, layer_z_bot );

    // Calculate an estimation for the nr of triangles based on the nr of objects
    unsigned int nrTrianglesEstimation = listObject2d.size() * 8;

    CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( nrTrianglesEstimation );

    // Load the 2D (X,Y axis) component of shapes
    for( LIST_OBJECT2D::const_iterator itemOnLayer = listObject2d.begin();
         itemOnLayer != listObject2d.end();
         ++itemOnLayer )
    {
        const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

        switch( object2d_A->GetObjectType() )
        {
        case OBJ2D_FILLED_CIRCLE:
            add_object_to_triangle_layer( (const CFILLEDCIRCLE2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJ2D_POLYGON4PT:
            add_object_to_triangle_layer( (const CPOLYGON4PTS2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJ2D_RING:
            add_object_to_triangle_layer( (const CRING2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJ2D_TRIANGLE:
            add_object_to_triangle_layer( (const CTRIANGLE2D *)object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJ2D_ROUNDSEG:
            add_object_to_triangle_layer( (const CROUNDSEGMENT2D *) object2d_A,
                                          layerTriangles, layer_z_top, layer_z_bot );
            break;

        default:
            wxFAIL_MSG("C3D_RENDER_OGL_LEGACY: Object type is not implemented");
            break;
        }
    }

    // Load the vertical (Z axis)  component of shapes
    if( layerPoly->OutlineCount() > 0 )
        layerTriangles->AddToMiddleContourns( *layerPoly, layer_z_bot, layer_z_top,
                                              biu_to_3d_units, false );

    // Create display list
    // /////////////////////////////////////////////////////////////////////
    /*m_ogl_disp_lists_layers[layer_id] = new CLAYERS_OGL_DISP_LISTS( *layerTriangles,
                                                                    m_ogl_circle_texture,
                                                                    layer_z_bot,
                                                                    layer_z_top );*/
    return layerTriangles;
}
