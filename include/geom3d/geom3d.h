#ifndef GEOM3D_H_
#define GEOM3D_H_

#include "class_board.h"
#include "clayer_triangles.h"

CLAYER_TRIANGLES* generate_3D_Vias( const BOARD* board );

CLAYER_TRIANGLES* generate_3D_layer( const BOARD* board, const PCB_LAYER_ID layer_id );

#endif // GEOM3D_H_
