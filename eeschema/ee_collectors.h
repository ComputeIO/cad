/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2011-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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

#ifndef EE_COLLECTORS_H
#define EE_COLLECTORS_H

#include <class_libentry.h>
#include <collector.h>
#include <dialogs/dialog_schematic_find.h>
#include <sch_item.h>
#include <sch_sheet_path.h>


/**
 * Class EE_COLLECTOR
 */
class EE_COLLECTOR : public COLLECTOR
{
public:
    static const KICAD_T AllItems[];
    static const KICAD_T EditableItems[];
    static const KICAD_T ComponentsOnly[];
    static const KICAD_T SheetsOnly[];

    EE_COLLECTOR( const KICAD_T* aScanTypes = EE_COLLECTOR::AllItems ) :
        m_Unit( 0 ),
        m_Convert( 0 )
    {
        SetScanTypes( aScanTypes );
    }

    /**
     * Overload COLLECTOR::operator[](int) to return a SCH_ITEM instead of an EDA_ITEM.
     *
     * @param aIndex The index into the list.
     * @return SCH_ITEM* at \a aIndex or NULL.
     */
    SCH_ITEM* operator[]( int aIndex ) const override
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )
            return (SCH_ITEM*) m_List[ aIndex ];

        return NULL;
    }

    SEARCH_RESULT Inspect( EDA_ITEM* aItem, void* aTestData ) override;

    /**
     * Function Collect
     * scans a EDA_ITEM using this class's Inspector method, which does the collection.
     * @param aScreen The eeschema screen to use for scanning
     * @param aFilterList A list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPos A wxPoint to use in hit-testing.
     * @param aUnit A symbol unit filter (for symbol editor)
     * @param aConvert A DeMorgan filter (for symbol editor)
     */
    void Collect( SCH_SCREEN* aScreen, const KICAD_T aFilterList[], const wxPoint& aPos,
            int aUnit = 0, int aConvert = 0 );

    /**
     * Function Collect
     * scans a EDA_ITEM using this class's Inspector method, which does the collection.
     * @param aItems The LIB_PART multivector holding the part components
     * @param aFilterList A list of #KICAD_T types with a terminating #EOT, that determines
     *                    what is to be collected and the priority order of the resulting
     *                    collection.
     * @param aPos A wxPoint to use in hit-testing.
     * @param aUnit A symbol unit filter (for symbol editor)
     * @param aConvert A DeMorgan filter (for symbol editor)
     */
    void Collect( LIB_ITEMS_CONTAINER& aItems, const KICAD_T aFilterList[], const wxPoint& aPos,
            int aUnit = 0, int aConvert = 0 );

    /**
     * Function IsCorner
     * tests if the collected items forms as corner of two line segments.
     * @return True if the collected items form a corner of two line segments.
     */
    bool IsCorner() const;

    /**
     * Function IsDraggableJunction
     * tests to see if the collected items form a draggable junction.
     * <p>
     * Daggable junctions are defined as:
     * <ul>
     * <li> The intersection of three or more wire end points. </li>
     * <li> The intersection of one or more wire end point and one wire mid point. </li>
     * <li> The crossing of two or more wire mid points and a junction. </li>
     * </ul>
     * </p>
     * @return True if the collection is a draggable junction.
     */
    bool IsDraggableJunction() const;

public:
    int      m_Unit;            // Fixed symbol unit filter (for symbol editor)
    int      m_Convert;         // Fixed DeMorgan filter (for symbol editor)
};


#endif // EE_COLLECTORS_H
