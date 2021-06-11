/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __KICAD_TYPEINFO_H
#define __KICAD_TYPEINFO_H

#include <iostream>

#ifndef SWIG
#include <type_traits>

/**
 * Check if the type of aObject is T.
 *
 * @param aObject the object for type check.
 * @return true, if aObject type equals T.
 */
template <class T, class I>
bool IsA( const I* aObject )
{
    return aObject && std::remove_pointer<T>::type::ClassOf( aObject );
}

template <class T, class I>
bool IsA( const I& aObject )
{
    return std::remove_pointer<T>::type::ClassOf( &aObject );
}

/**
 * A lightweight dynamic downcast.
 *
 * Cast \a aObject to type Casted*.  Uses #EDA_ITEM::Type() and #EDA_ITEM::ClassOf() to
 * check if type matches.
 *
 * @param aObject object to be casted.
 * @return down-casted object or NULL if type doesn't match Casted.
 */
template<class Casted, class From>
Casted dyn_cast( From aObject )
{
    if( std::remove_pointer<Casted>::type::ClassOf ( aObject ) )
        return static_cast<Casted>( aObject );

    return nullptr;
}

class EDA_ITEM;

#endif  // SWIG


/**
 * The set of class identification values stored in #EDA_ITEM::m_structType
 */
enum KICAD_T
{
    NOT_USED = -1, ///< the 3d code uses this value

    EOT = 0, ///< search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T, ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_FOOTPRINT_T,        ///< class FOOTPRINT, a footprint
    PCB_PAD_T,              ///< class PAD, a pad in a footprint
    PCB_SHAPE_T,            ///< class PCB_SHAPE, a segment not on copper layers
    PCB_TEXT_T,             ///< class PCB_TEXT, text on a layer
    PCB_FP_TEXT_T,          ///< class FP_TEXT, text in a footprint
    PCB_FP_SHAPE_T,         ///< class FP_SHAPE, a footprint edge
    PCB_FP_ZONE_T,          ///< class ZONE, managed by a footprint
    PCB_TRACE_T,            ///< class TRACK, a track segment (segment on a copper layer)
    PCB_VIA_T,              ///< class VIA, a via (like a track segment on a copper layer)
    PCB_ARC_T,              ///< class ARC, an arc track segment on a copper layer
    PCB_MARKER_T,           ///< class MARKER_PCB, a marker used to show something
    PCB_DIMENSION_T,        ///< class DIMENSION_BASE: abstract dimension meta-type
    PCB_DIM_ALIGNED_T,      ///< class ALIGNED_DIMENSION, a linear dimension (graphic item)
    PCB_DIM_LEADER_T,       ///< class LEADER, a leader dimension (graphic item)
    PCB_DIM_CENTER_T,       ///< class CENTER_DIMENSION, a center point marking (graphic item)
    PCB_DIM_ORTHOGONAL_T,   ///< class ORTHOGONAL_DIMENSION, a linear dimension constrained to x/y
    PCB_TARGET_T,           ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_T,             ///< class ZONE, a copper pour area
    PCB_ITEM_LIST_T,        ///< class BOARD_ITEM_LIST, a list of board items
    PCB_NETINFO_T,          ///< class NETINFO_ITEM, a description of a net
    PCB_GROUP_T,            ///< class PCB_GROUP, a set of BOARD_ITEMs

    PCB_LOCATE_STDVIA_T,
    PCB_LOCATE_UVIA_T,
    PCB_LOCATE_BBVIA_T,
    PCB_LOCATE_TEXT_T,
    PCB_LOCATE_GRAPHIC_T,
    PCB_LOCATE_HOLE_T,
    PCB_LOCATE_PTH_T,
    PCB_LOCATE_NPTH_T,
    PCB_LOCATE_BOARD_EDGE_T,

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currently ordered to mimic the old Eeschema locate behavior where
    // the smallest item is the selected item.
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_PIN_T,

    // Be prudent with these types:
    // they should be used only to locate a specific field type among SCH_FIELD_Ts
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    SCH_FIELD_LOCATE_REFERENCE_T,
    SCH_FIELD_LOCATE_VALUE_T,
    SCH_FIELD_LOCATE_FOOTPRINT_T,
    SCH_FIELD_LOCATE_DATASHEET_T,

    // Same for picking wires and buses from SCH_LINE_T items
    SCH_LINE_LOCATE_WIRE_T,
    SCH_LINE_LOCATE_BUS_T,
    SCH_LINE_LOCATE_GRAPHIC_LINE_T,

    // Same for picking labels attached to wires and/or buses
    SCH_LABEL_LOCATE_WIRE_T,
    SCH_LABEL_LOCATE_BUS_T,

    // Same for picking symbols which are power symbols
    SCH_SYMBOL_LOCATE_POWER_T,

    // matches any type
    SCH_LOCATE_ANY_T,

    // General
    SCH_SCREEN_T,

    SCHEMATIC_T,

    /*
     * Draw items in library symbol.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the symbol definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_PART_T,
    LIB_ALIAS_T,
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,

    /*
     * Fields are not saved inside the "DRAW/ENDDRAW".  Add new draw item
     * types before this line.
     */
    LIB_FIELD_T,

    /*
     * For GerbView: item types:
     */
    GERBER_LAYOUT_T,
    GERBER_DRAW_ITEM_T,
    GERBER_IMAGE_T,

    /*
     * For Pl_Editor: item types:
     */
    WSG_LINE_T,
    WSG_RECT_T,
    WSG_POLY_T,
    WSG_TEXT_T,
    WSG_BITMAP_T,
    WSG_PAGE_T,

    // serialized layout used in undo/redo commands
    WS_PROXY_UNDO_ITEM_T,      // serialized layout used in undo/redo commands
    WS_PROXY_UNDO_ITEM_PLUS_T, // serialized layout plus page and title block settings

    /*
     * FOR PROJECT::_ELEMs
     */
    SYMBOL_LIB_TABLE_T,
    FP_LIB_TABLE_T,
    PART_LIBS_T,
    SEARCH_STACK_T,
    S3D_CACHE_T,

    // End value
    MAX_STRUCT_TYPE_ID
};

/**
 * Return the underlying type of the given type.
 *
 * This is useful for finding the element type given one of the "non-type" types such as
 * SCH_LINE_LOCATE_WIRE_T.
 *
 * @param aType Given type to resolve.
 * @return Base type.
 */
constexpr KICAD_T BaseType( const KICAD_T aType )
{
    switch( aType )
    {
    case SCH_FIELD_LOCATE_REFERENCE_T:
    case SCH_FIELD_LOCATE_VALUE_T:
    case SCH_FIELD_LOCATE_FOOTPRINT_T:
    case SCH_FIELD_LOCATE_DATASHEET_T:
        return SCH_FIELD_T;

    case SCH_LINE_LOCATE_WIRE_T:
    case SCH_LINE_LOCATE_BUS_T:
    case SCH_LINE_LOCATE_GRAPHIC_LINE_T:
        return SCH_LINE_T;

    case SCH_LABEL_LOCATE_WIRE_T:
    case SCH_LABEL_LOCATE_BUS_T:
        return SCH_LABEL_T;

    case SCH_SYMBOL_LOCATE_POWER_T:
        return SCH_SYMBOL_T;

    case PCB_LOCATE_HOLE_T:
    case PCB_LOCATE_PTH_T:
    case PCB_LOCATE_NPTH_T:
        return PCB_LOCATE_HOLE_T;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        return PCB_DIMENSION_T;

    default:
        return aType;
    }
}


inline std::ostream& operator<<( std::ostream& os, KICAD_T aType )
{
    switch( aType )
    {
    case KICAD_T::NOT_USED: os << "NOT_USED"; break;
    //case KICAD_T::EOT: os << "EOT"; break;
    case KICAD_T::TYPE_NOT_INIT: os << "EOT/TYPE_NOT_INIT"; break;
    case KICAD_T::PCB_T: os << "PCB_T"; break;
    case KICAD_T::SCREEN_T: os << "SCREEN_T"; break;

    // Items in pcb
    case KICAD_T::PCB_FOOTPRINT_T: os << "PCB_FOOTPRINT_T"; break;
    case KICAD_T::PCB_PAD_T: os << "PCB_PAD_T"; break;
    case KICAD_T::PCB_SHAPE_T: os << "PCB_SHAPE_T"; break;
    case KICAD_T::PCB_TEXT_T: os << "PCB_TEXT_T"; break;
    case KICAD_T::PCB_FP_TEXT_T: os << "PCB_FP_TEXT_T"; break;
    case KICAD_T::PCB_FP_SHAPE_T: os << "PCB_FP_SHAPE_T"; break;
    case KICAD_T::PCB_FP_ZONE_T: os << "PCB_FP_ZONE_T"; break;
    case KICAD_T::PCB_TRACE_T: os << "PCB_TRACE_T"; break;
    case KICAD_T::PCB_VIA_T: os << "PCB_VIA_T"; break;
    case KICAD_T::PCB_ARC_T: os << "PCB_ARC_T"; break;
    case KICAD_T::PCB_MARKER_T: os << "PCB_MARKER_T"; break;
    case KICAD_T::PCB_DIMENSION_T: os << "PCB_DIMENSION_T"; break;
    case KICAD_T::PCB_DIM_ALIGNED_T: os << "PCB_DIM_ALIGNED_T"; break;
    case KICAD_T::PCB_DIM_LEADER_T: os << "PCB_DIM_LEADER_T"; break;
    case KICAD_T::PCB_DIM_CENTER_T: os << "PCB_DIM_CENTER_T"; break;
    case KICAD_T::PCB_DIM_ORTHOGONAL_T: os << "PCB_DIM_ORTHOGONAL_T"; break;
    case KICAD_T::PCB_TARGET_T: os << "PCB_TARGET_T"; break;
    case KICAD_T::PCB_ZONE_T: os << "PCB_ZONE_T"; break;
    case KICAD_T::PCB_ITEM_LIST_T: os << "PCB_ITEM_LIST_T"; break;
    case KICAD_T::PCB_NETINFO_T: os << "PCB_NETINFO_T"; break;
    case KICAD_T::PCB_GROUP_T: os << "PCB_GROUP_T"; break;

    case KICAD_T::PCB_LOCATE_STDVIA_T: os << "PCB_LOCATE_STDVIA_T"; break;
    case KICAD_T::PCB_LOCATE_UVIA_T: os << "PCB_LOCATE_UVIA_T"; break;
    case KICAD_T::PCB_LOCATE_BBVIA_T: os << "PCB_LOCATE_BBVIA_T"; break;
    case KICAD_T::PCB_LOCATE_TEXT_T: os << "PCB_LOCATE_TEXT_T"; break;
    case KICAD_T::PCB_LOCATE_GRAPHIC_T: os << "PCB_LOCATE_GRAPHIC_T"; break;
    case KICAD_T::PCB_LOCATE_HOLE_T: os << "PCB_LOCATE_HOLE_T"; break;
    case KICAD_T::PCB_LOCATE_PTH_T: os << "PCB_LOCATE_PTH_T"; break;
    case KICAD_T::PCB_LOCATE_NPTH_T: os << "PCB_LOCATE_NPTH_T"; break;
    case KICAD_T::PCB_LOCATE_BOARD_EDGE_T: os << "PCB_LOCATE_BOARD_EDGE_T"; break;

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currently ordered to mimic the old Eeschema locate behavior where
    // the smallest item is the selected item.
    case KICAD_T::SCH_MARKER_T: os << "SCH_MARKER_T"; break;
    case KICAD_T::SCH_JUNCTION_T: os << "SCH_JUNCTION_T"; break;
    case KICAD_T::SCH_NO_CONNECT_T: os << "SCH_NO_CONNECT_T"; break;
    case KICAD_T::SCH_BUS_WIRE_ENTRY_T: os << "SCH_BUS_WIRE_ENTRY_T"; break;
    case KICAD_T::SCH_BUS_BUS_ENTRY_T: os << "SCH_BUS_BUS_ENTRY_T"; break;
    case KICAD_T::SCH_LINE_T: os << "SCH_LINE_T"; break;
    case KICAD_T::SCH_BITMAP_T: os << "SCH_BITMAP_T"; break;
    case KICAD_T::SCH_TEXT_T: os << "SCH_TEXT_T"; break;
    case KICAD_T::SCH_LABEL_T: os << "SCH_LABEL_T"; break;
    case KICAD_T::SCH_GLOBAL_LABEL_T: os << "SCH_GLOBAL_LABEL_T"; break;
    case KICAD_T::SCH_HIER_LABEL_T: os << "SCH_HIER_LABEL_T"; break;
    case KICAD_T::SCH_FIELD_T: os << "SCH_FIELD_T"; break;
    case KICAD_T::SCH_COMPONENT_T: os << "SCH_COMPONENT_T"; break;
    case KICAD_T::SCH_SHEET_PIN_T: os << "SCH_SHEET_PIN_T"; break;
    case KICAD_T::SCH_SHEET_T: os << "SCH_SHEET_T"; break;
    case KICAD_T::SCH_PIN_T: os << "SCH_PIN_T"; break;

    // Be prudent with these types:
    // they should be used only to locate a specific field type among SCH_FIELD_Ts
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    case KICAD_T::SCH_FIELD_LOCATE_REFERENCE_T: os << "SCH_FIELD_LOCATE_REFERENCE_T"; break;
    case KICAD_T::SCH_FIELD_LOCATE_VALUE_T: os << "SCH_FIELD_LOCATE_VALUE_T"; break;
    case KICAD_T::SCH_FIELD_LOCATE_FOOTPRINT_T: os << "SCH_FIELD_LOCATE_FOOTPRINT_T"; break;
    case KICAD_T::SCH_FIELD_LOCATE_DATASHEET_T: os << "SCH_FIELD_LOCATE_DATASHEET_T"; break;

    // Same for picking wires and buses from SCH_LINE_T items
    case KICAD_T::SCH_LINE_LOCATE_WIRE_T: os << "SCH_LINE_LOCATE_WIRE_T"; break;
    case KICAD_T::SCH_LINE_LOCATE_BUS_T: os << "SCH_LINE_LOCATE_BUS_T"; break;
    case KICAD_T::SCH_LINE_LOCATE_GRAPHIC_LINE_T: os << "SCH_LINE_LOCATE_GRAPHIC_LINE_T"; break;

    // Same for picking labels attached to wires and/or buses
    case KICAD_T::SCH_LABEL_LOCATE_WIRE_T: os << "SCH_LABEL_LOCATE_WIRE_T"; break;
    case KICAD_T::SCH_LABEL_LOCATE_BUS_T: os << "SCH_LABEL_LOCATE_BUS_T"; break;

    // Same for picking components which are power symbols
    case KICAD_T::SCH_COMPONENT_LOCATE_POWER_T: os << "SCH_COMPONENT_LOCATE_POWER_T"; break;

    // matches any type
    case KICAD_T::SCH_LOCATE_ANY_T: os << "SCH_LOCATE_ANY_T"; break;

    // General
    case KICAD_T::SCH_SCREEN_T: os << "SCH_SCREEN_T"; break;

    case KICAD_T::SCHEMATIC_T: os << "SCHEMATIC_T"; break;

    /*
     * Draw items in library component.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the component definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    case KICAD_T::LIB_PART_T: os << "LIB_PART_T"; break;
    case KICAD_T::LIB_ALIAS_T: os << "LIB_ALIAS_T"; break;
    case KICAD_T::LIB_ARC_T: os << "LIB_ARC_T"; break;
    case KICAD_T::LIB_CIRCLE_T: os << "LIB_CIRCLE_T"; break;
    case KICAD_T::LIB_TEXT_T: os << "LIB_TEXT_T"; break;
    case KICAD_T::LIB_RECTANGLE_T: os << "LIB_RECTANGLE_T"; break;
    case KICAD_T::LIB_POLYLINE_T: os << "LIB_POLYLINE_T"; break;
    case KICAD_T::LIB_BEZIER_T: os << "LIB_BEZIER_T"; break;
    case KICAD_T::LIB_PIN_T: os << "LIB_PIN_T"; break;

    /*
     * Fields are not saved inside the "DRAW/ENDDRAW".  Add new draw item
     * types before this line.
     */
    case KICAD_T::LIB_FIELD_T: os << "LIB_FIELD_T"; break;

    /*
     * For GerbView: item types:
     */
    case KICAD_T::GERBER_LAYOUT_T: os << "GERBER_LAYOUT_T"; break;
    case KICAD_T::GERBER_DRAW_ITEM_T: os << "GERBER_DRAW_ITEM_T"; break;
    case KICAD_T::GERBER_IMAGE_T: os << "GERBER_IMAGE_T"; break;

    /*
     * For Pl_Editor: item types:
     */
    case KICAD_T::WSG_LINE_T: os << "WSG_LINE_T"; break;
    case KICAD_T::WSG_RECT_T: os << "WSG_RECT_T"; break;
    case KICAD_T::WSG_POLY_T: os << "WSG_POLY_T"; break;
    case KICAD_T::WSG_TEXT_T: os << "WSG_TEXT_T"; break;
    case KICAD_T::WSG_BITMAP_T: os << "WSG_BITMAP_T"; break;
    case KICAD_T::WSG_PAGE_T: os << "WSG_PAGE_T"; break;

    // serialized layout used in undo/redo commands
    case KICAD_T::WS_PROXY_UNDO_ITEM_T: os << "WS_PROXY_UNDO_ITEM_T"; break;
    case KICAD_T::WS_PROXY_UNDO_ITEM_PLUS_T: os << "WS_PROXY_UNDO_ITEM_PLUS_T"; break;

    /*
     * FOR PROJECT::_ELEMs
     */
    case KICAD_T::SYMBOL_LIB_TABLE_T: os << "SYMBOL_LIB_TABLE_T"; break;
    case KICAD_T::FP_LIB_TABLE_T: os << "FP_LIB_TABLE_T"; break;
    case KICAD_T::PART_LIBS_T: os << "PART_LIBS_T"; break;
    case KICAD_T::SEARCH_STACK_T: os << "SEARCH_STACK_T"; break;
    case KICAD_T::S3D_CACHE_T: os << "S3D_CACHE_T"; break;

    // End value
    case KICAD_T::MAX_STRUCT_TYPE_ID: os << "MAX_STRUCT_TYPE_ID"; break;
    default: os << "*Unknown*";
    }

    return os;
}

#endif // __KICAD_TYPEINFO_H
