#include "class_board.h"
#include "3d_render_ogl_legacy/clayer_triangles.h"

CLAYER_TRIANGLES* generate_3D_Vias( const BOARD* board );

CLAYER_TRIANGLES* generate_3D_layer( const BOARD* board, const PCB_LAYER_ID layer_id );
