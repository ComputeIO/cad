/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>

#include <core/kicad_algo.h>
#include <eeschema_id.h>
#include <general.h>
#include <lib_item.h>
#include <sch_bus_entry.h>
#include <sch_component.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>


void SCH_EDIT_FRAME::GetSchematicConnections( std::vector< wxPoint >& aConnections )
{
    for( auto item : GetScreen()->Items() )
    {
        // Avoid items that are changing
        if( !( item->GetEditFlags() & ( IS_DRAGGED | IS_MOVED | IS_DELETED ) ) )
            item->GetConnectionPoints( aConnections );
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( aConnections.begin(), aConnections.end(),
            []( const wxPoint& a, const wxPoint& b ) -> bool
            { return a.x < b.x || (a.x == b.x && a.y < b.y); } );
    aConnections.erase(
            std::unique( aConnections.begin(), aConnections.end() ), aConnections.end() );
}


bool SCH_EDIT_FRAME::TestDanglingEnds()
{
    std::vector<DANGLING_END_ITEM> endPoints;
    bool hasStateChanged = false;

    for( auto item : GetScreen()->Items() )
        item->GetEndPoints( endPoints );

    for( auto item : GetScreen()->Items() )
    {
        if( item->UpdateDanglingState( endPoints ) )
        {
            GetCanvas()->GetView()->Update( item, KIGFX::REPAINT );
            hasStateChanged = true;
        }
        item->GetEndPoints( endPoints );
    }

    return hasStateChanged;
}


bool SCH_EDIT_FRAME::TrimWire( const wxPoint& aStart, const wxPoint& aEnd )
{
    bool retval = false;

    if( aStart == aEnd )
        return retval;

    for( auto item : GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() != LAYER_WIRE )
            continue;

        // Don't remove wires that are already deleted or are currently being dragged
        if( line->GetEditFlags() & ( STRUCT_DELETED | IS_DRAGGED | IS_MOVED | SKIP_STRUCT ) )
            continue;

        if( !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aStart ) ||
                !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aEnd ) )
        {
            continue;
        }

        // Don't remove entire wires
        if( ( line->GetStartPoint() == aStart && line->GetEndPoint() == aEnd )
            || ( line->GetStartPoint() == aEnd && line->GetEndPoint() == aStart ) )
        {
            continue;
        }

        // Step 1: break the segment on one end.  return_line remains line if not broken.
        // Ensure that *line points to the segment containing aEnd
        SCH_LINE* return_line = line;
        BreakSegment( line, aStart, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aEnd ) )
            line = return_line;

        // Step 2: break the remaining segment.  return_line remains line if not broken.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        BreakSegment( line, aEnd, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aStart ) )
            line = return_line;

        SaveCopyInUndoList( line, UR_DELETED, true );
        RemoveFromScreen( line );

        retval = true;
    }

    return retval;
}


bool SCH_EDIT_FRAME::SchematicCleanUp( SCH_SCREEN* aScreen )
{
    PICKED_ITEMS_LIST            itemList;
    EE_SELECTION_TOOL*           selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::vector<SCH_ITEM*>       deletedItems;
    std::vector<SCH_LINE*>       lines;
    std::vector<SCH_JUNCTION*>   junctions;
    std::vector<SCH_NO_CONNECT*> ncs;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    auto remove_item = [&itemList, &deletedItems]( SCH_ITEM* aItem ) -> void {
        aItem->SetFlags( STRUCT_DELETED );
        itemList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
        deletedItems.push_back( aItem );
    };

    BreakSegmentsOnJunctions( aScreen );

    for( auto item : aScreen->Items().OfType( SCH_LINE_T ) )
    {
        if( item->GetLayer() == LAYER_WIRE )
            lines.push_back( static_cast<SCH_LINE*>( item ) );
    }

    for( auto item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
    {
        if( !aScreen->IsJunctionNeeded( item->GetPosition() ) )
            remove_item( item );
        else
            junctions.push_back( static_cast<SCH_JUNCTION*>( item ) );
    }

    for( auto item : aScreen->Items().OfType( SCH_NO_CONNECT_T ) )
    {
        ncs.push_back( static_cast<SCH_NO_CONNECT*>( item ) );
    }

    alg::for_all_pairs(
            junctions.begin(), junctions.end(), [&]( SCH_JUNCTION* aFirst, SCH_JUNCTION* aSecond ) {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                    return;

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );

    alg::for_all_pairs(
            ncs.begin(), ncs.end(), [&]( SCH_NO_CONNECT* aFirst, SCH_NO_CONNECT* aSecond ) {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                    return;

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );

    for( auto it1 = lines.begin(); it1 != lines.end(); ++it1 )
    {
        SCH_LINE* firstLine = *it1;

        if( firstLine->GetEditFlags() & STRUCT_DELETED )
            continue;

        if( firstLine->IsNull() )
        {
            remove_item( firstLine );
            continue;
        }

        auto it2 = it1;

        for( ++it2; it2 != lines.end(); ++it2 )
        {
            SCH_LINE* secondLine = *it2;
            bool      needed     = false;

            if( secondLine->GetFlags() & STRUCT_DELETED )
                continue;

            if( !secondLine->IsParallel( firstLine )
                    || secondLine->GetLineStyle() != firstLine->GetLineStyle()
                    || secondLine->GetLineColor() != firstLine->GetLineColor()
                    || secondLine->GetLineSize() != firstLine->GetLineSize() )
                continue;

            // Remove identical lines
            if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                    && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
            {
                remove_item( secondLine );
                continue;
            }

            // If the end points overlap, check if we still need the junction
            if( secondLine->IsEndPoint( firstLine->GetStartPoint() ) )
                needed = aScreen->IsJunctionNeeded( firstLine->GetStartPoint() );
            else if( secondLine->IsEndPoint( firstLine->GetEndPoint() ) )
                needed = aScreen->IsJunctionNeeded( firstLine->GetEndPoint() );

            SCH_LINE* mergedLine = nullptr;

            if( !needed && ( mergedLine = secondLine->MergeOverlap( firstLine ) ) )
            {
                remove_item( firstLine );
                remove_item( secondLine );
                itemList.PushItem( ITEM_PICKER( mergedLine, UR_NEW ) );

                AddToScreen( mergedLine, aScreen );

                if( firstLine->IsSelected() )
                    selectionTool->AddItemToSel( mergedLine, true /*quiet mode*/ );

                break;
            }
        }
    }


    for( auto item : deletedItems )
    {
        if( item->IsSelected() )
            selectionTool->RemoveItemFromSel( item, true /*quiet mode*/ );

        RemoveFromScreen( item, aScreen );
    }

    if( itemList.GetCount() )
        SaveCopyInUndoList( itemList, UR_DELETED, true );

    return itemList.GetCount() > 0;
}


bool SCH_EDIT_FRAME::BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint,
                                   SCH_LINE** aNewSegment, SCH_SCREEN* aScreen )
{
    if( !IsPointOnSegment( aSegment->GetStartPoint(), aSegment->GetEndPoint(), aPoint )
            || aSegment->IsEndPoint( aPoint ) )
        return false;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    SCH_LINE* newSegment = new SCH_LINE( *aSegment );

    newSegment->SetStartPoint( aPoint );
    AddToScreen( newSegment, aScreen );

    SaveCopyInUndoList( newSegment, UR_NEW, true );
    SaveCopyInUndoList( aSegment, UR_CHANGED, true );

    RefreshItem( aSegment );
    aSegment->SetEndPoint( aPoint );

    if( aNewSegment )
        *aNewSegment = newSegment;

    return true;
}


bool SCH_EDIT_FRAME::BreakSegments( const wxPoint& aPoint, SCH_SCREEN* aScreen )
{
    static const KICAD_T wiresAndBusses[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool                   brokenSegments = false;
    std::vector<SCH_LINE*> wires;
    EDA_RECT               bbox( aPoint, wxSize( 2, 2 ) );

    for( auto item : aScreen->Items().Overlapping( SCH_LINE_T, aPoint ) )
    {
        if( item->IsType( wiresAndBusses ) )
            wires.push_back( static_cast<SCH_LINE*>( item ) );
    }

    for( auto wire : wires )
        brokenSegments |= BreakSegment( wire, aPoint, NULL, aScreen );

    return brokenSegments;
}


bool SCH_EDIT_FRAME::BreakSegmentsOnJunctions( SCH_SCREEN* aScreen )
{
    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool brokenSegments = false;

    std::set<wxPoint> point_set;
    for( auto item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
        point_set.insert( item->GetPosition() );

    for( auto item : aScreen->Items().OfType( SCH_BUS_WIRE_ENTRY_T ) )
    {
        auto entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );
        point_set.insert( entry->GetPosition() );
        point_set.insert( entry->m_End() );
    }


    for( auto pt : point_set )
        brokenSegments |= BreakSegments( pt, aScreen );

    return brokenSegments;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_ITEM* aJunction, bool aAppend )
{
    SCH_SCREEN*        screen = GetScreen();
    PICKED_ITEMS_LIST  undoList;
    EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    KICAD_T            wiresAndBusses[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    auto remove_item = [ & ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        undoList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
    };

    remove_item( aJunction );
    RemoveFromScreen( aJunction );

    /// Note that std::list or similar is required here as we may insert values in the
    /// loop below.  This will invalidate iterators in a std::vector or std::deque
    std::list<SCH_LINE*> lines;

    for( auto item : screen->Items().Overlapping( SCH_LINE_T, aJunction->GetPosition() ) )
    {
        auto line = static_cast<SCH_LINE*>( item );

        if( line->IsType( wiresAndBusses ) && line->IsEndPoint( aJunction->GetPosition() )
                && !( line->GetEditFlags() & STRUCT_DELETED ) )
            lines.push_back( line );
    }

    alg::for_all_pairs(
            lines.begin(), lines.end(), [&]( SCH_LINE* firstLine, SCH_LINE* secondLine ) {
                if( ( firstLine->GetEditFlags() & STRUCT_DELETED )
                        || ( secondLine->GetEditFlags() & STRUCT_DELETED )
                        || !secondLine->IsParallel( firstLine ) )
                    return;

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                        && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( firstLine );
                    return;
                }

                // Try to merge the remaining lines
                if( SCH_LINE* line = secondLine->MergeOverlap( firstLine ) )
                {
                    remove_item( firstLine );
                    remove_item( secondLine );
                    undoList.PushItem( ITEM_PICKER( line, UR_NEW ) );
                    AddToScreen( line );

                    if( line->IsSelected() )
                        selectionTool->AddItemToSel( line, true /*quiet mode*/ );

                    lines.push_back( line );
                }
            } );

    SaveCopyInUndoList( undoList, UR_DELETED, aAppend );


    for( auto line : lines )
    {
        if( line->GetEditFlags() & STRUCT_DELETED )
        {
            if( line->IsSelected() )
                selectionTool->RemoveItemFromSel( line, true /*quiet mode*/ );

            RemoveFromScreen( line );
        }
    }
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( const wxPoint& aPos, bool aUndoAppend, bool aFinal )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPos );

    AddToScreen( junction );
    SaveCopyInUndoList( junction, UR_NEW, aUndoAppend );
    BreakSegments( aPos );

    if( aFinal )
    {
        m_toolManager->PostEvent( EVENTS::SelectedItemsModified );

        TestDanglingEnds();
        OnModify();

        auto view = GetCanvas()->GetView();
        view->ClearPreview();
        view->ShowPreview( false );
        view->ClearHiddenFlags();
    }

    return junction;
}


