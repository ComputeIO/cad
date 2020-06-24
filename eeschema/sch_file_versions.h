/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * This file contains the file format version information for the s-expression schematic
 * and symbol library file formats.
 *
 * @note Comment out the last version and add the new version as a date time stamp in the
 *       YYYYMMDD format.  Comment the changes to the file format for historical purposes.
 *
 */

/**
 * Symbol library file version.
 */
#define SEXPR_SYMBOL_LIB_FILE_VERSION  20200126  // Initial version.

/**
 * Symbol library file version.
 */
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200310  // Initial version.  Sheet fields were named
                                                 // incorectly (using symbol field vocabulary).

//#define SEXPR_SCHEMATIC_FILE_VERSION 20200506  // Used "page" instead of "paper" for paper
                                                 // sizes.

//#define SEXPR_SCHEMATIC_FILE_VERSION 20200512  // Add support for exclude from BOM.

//#define SEXPR_SCHEMATIC_FILE_VERSION 20200602  // Add support for exclude from board.

//#define SEXPR_SCHEMATIC_FILE_VERSION 20200608  // Add support for bus and junction properties.

#define SEXPR_SCHEMATIC_FILE_VERSION 20200618
