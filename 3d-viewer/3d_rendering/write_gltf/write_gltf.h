#ifndef WRITE_GLTF_H
#define WRITE_GLTF_H

#include <string>

#include "class_board.h"

/**
 * @brief WriteGLTF - Writes the specified layer of the board to a binary GLTF
 * file of the same name
 */
void WriteGLTF( const BOARD* board, const PCB_LAYER_ID layer_id, const std::string& out_dir ) __attribute__ ((visibility ("default") ));

#endif
