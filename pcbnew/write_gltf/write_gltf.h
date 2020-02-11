#ifndef WRITE_GLTF_H_
#define WRITE_GLTF_H_

#include <string>

#include "class_board.h"

/**
 * @brief WriteGLTF - Writes the specified layer of the board to a binary GLTF
 * file of the same name
 */
void WriteGLTF( const BOARD* board, const PCB_LAYER_ID layer_id, const wxString& out_dir );

#endif
