/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <schematic.h>

#include <sch_reference_list.h>
#include <class_library.h>


void SCH_EDIT_FRAME::mapExistingAnnotation( std::map<wxString, wxString>& aMap )
{
    SCH_REFERENCE_LIST references;

    Schematic().GetSheets().GetComponents( references );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_COMPONENT* comp = references[ i ].GetComp();
        SCH_SHEET_PATH* curr_sheetpath = &references[ i ].GetSheetPath();
        KIID_PATH curr_full_uuid = curr_sheetpath->Path();
        curr_full_uuid.push_back( comp->m_Uuid );

        wxString ref = comp->GetRef( curr_sheetpath );

        if( comp->GetUnitCount() > 1 )
            ref << LIB_PART::SubReference( comp->GetUnitSelection( curr_sheetpath ) );

        if( comp->IsAnnotated( curr_sheetpath ) )
            aMap[ curr_full_uuid.AsString() ] = ref;
    }
}


void SCH_EDIT_FRAME::DeleteAnnotation( bool aCurrentSheetOnly, bool* aAppendUndo )
{
    auto clearAnnotation =
            [&]( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet )
            {
                for( SCH_ITEM* item : aScreen->Items().OfType( SCH_COMPONENT_T ) )
                {
                    SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

                    SaveCopyInUndoList( aScreen, component, UR_CHANGED, *aAppendUndo );
                    *aAppendUndo = true;
                    component->ClearAnnotation( aSheet );

                    // Clear the modified component flag set by component->ClearAnnotation
                    // because we do not use it here and we should not leave this flag set,
                    // when an editing is finished:
                    component->ClearFlags();
                }
            };

    if( aCurrentSheetOnly )
    {
        clearAnnotation( GetScreen(), &GetCurrentSheet() );
    }
    else
    {
        for( const SCH_SHEET_PATH& sheet : Schematic().GetSheets() )
            clearAnnotation( sheet.LastScreen(), nullptr );
    }

    // Update the references for the sheet that is currently being displayed.
    GetCurrentSheet().UpdateAllScreenReferences();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();
}


void SCH_EDIT_FRAME::AnnotateComponents( bool              aAnnotateSchematic,
                                         ANNOTATE_ORDER_T  aSortOption,
                                         ANNOTATE_OPTION_T aAlgoOption,
                                         int               aStartNumber,
                                         bool              aResetAnnotation,
                                         bool              aRepairTimestamps,
                                         bool              aLockUnits,
                                         REPORTER&         aReporter )
{
    SCH_REFERENCE_LIST references;
    SCH_SCREENS        screens( Schematic().Root() );
    SCH_SHEET_LIST     sheets = Schematic().GetSheets();
    bool               appendUndo = false;

    // Map of locked components
    SCH_MULTI_UNIT_REFERENCE_MAP lockedComponents;

    // Map of previous annotation for building info messages
    std::map<wxString, wxString> previousAnnotation;

    // Test for and replace duplicate time stamps in components and sheets.  Duplicate
    // time stamps can happen with old schematics, schematic conversions, or manual
    // editing of files.
    if( aRepairTimestamps )
    {
        int count = screens.ReplaceDuplicateTimeStamps();

        if( count )
        {
            wxString msg;
            msg.Printf( _( "%d duplicate time stamps were found and replaced." ), count );
            aReporter.ReportTail( msg, RPT_SEVERITY_WARNING );
        }
    }

    // If units must be locked, collect all the sets that must be annotated together.
    if( aLockUnits )
    {
        if( aAnnotateSchematic )
            sheets.GetMultiUnitComponents( lockedComponents );
        else
            GetCurrentSheet().GetMultiUnitComponents( lockedComponents );
    }

    // Store previous annotations for building info messages
    mapExistingAnnotation( previousAnnotation );

    // If it is an annotation for all the components, reset previous annotation.
    if( aResetAnnotation )
        DeleteAnnotation( !aAnnotateSchematic, &appendUndo );

    // Set sheet number and number of sheets.
    SetSheetNumberAndCount();

    // Build component list
    if( aAnnotateSchematic )
        sheets.GetComponents( references );
    else
        GetCurrentSheet().GetComponents( references );

    // Break full components reference in name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    switch( aSortOption )
    {
    default:
    case SORT_BY_X_POSITION: references.SortByXCoordinate(); break;
    case SORT_BY_Y_POSITION: references.SortByYCoordinate(); break;
    }

    bool useSheetNum = false;
    int idStep = 100;

    switch( aAlgoOption )
    {
    default:
    case INCREMENTAL_BY_REF:
        break;

    case SHEET_NUMBER_X_100:
        useSheetNum = true;
        break;

    case SHEET_NUMBER_X_1000:
        useSheetNum = true;
        idStep = 1000;
        break;
    }

    // Recalculate and update reference numbers in schematic
    references.Annotate( useSheetNum, idStep, aStartNumber, lockedComponents );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_REFERENCE&  ref = references[i];
        SCH_COMPONENT*  comp = ref.GetComp();
        SCH_SHEET_PATH* sheet = &ref.GetSheetPath();

        SaveCopyInUndoList( sheet->LastScreen(), comp, UR_CHANGED, appendUndo );
        appendUndo = true;
        ref.Annotate();

        KIID_PATH full_uuid = sheet->Path();
        full_uuid.push_back( comp->m_Uuid );

        wxString  prevRef = previousAnnotation[ full_uuid.AsString() ];
        wxString  newRef  = comp->GetRef( sheet );

        if( comp->GetUnitCount() > 1 )
            newRef << LIB_PART::SubReference( comp->GetUnitSelection( sheet ) );

        wxString msg;

        if( prevRef.Length() )
        {
            if( newRef == prevRef )
                continue;

            if( comp->GetUnitCount() > 1 )
                msg.Printf( _( "Updated %s (unit %s) from %s to %s" ),
                            comp->GetField( VALUE )->GetShownText(),
                            LIB_PART::SubReference( comp->GetUnit(), false ),
                            prevRef,
                            newRef );
            else
                msg.Printf( _( "Updated %s from %s to %s" ),
                            comp->GetField( VALUE )->GetShownText(),
                            prevRef,
                            newRef );
        }
        else
        {
            if( comp->GetUnitCount() > 1 )
                msg.Printf( _( "Annotated %s (unit %s) as %s" ),
                            comp->GetField( VALUE )->GetShownText(),
                            LIB_PART::SubReference( comp->GetUnit(), false ),
                            newRef );
            else
                msg.Printf( _( "Annotated %s as %s" ),
                            comp->GetField( VALUE )->GetShownText(),
                            newRef );
        }

        aReporter.Report( msg, RPT_SEVERITY_ACTION );
    }

    // Final control (just in case ... ).
    if( !CheckAnnotate( aReporter, !aAnnotateSchematic ) )
        aReporter.ReportTail( _( "Annotation complete." ), RPT_SEVERITY_ACTION );

    // Update on screen references, that can be modified by previous calculations:
    GetCurrentSheet().UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();
}


int SCH_EDIT_FRAME::CheckAnnotate( REPORTER& aReporter, bool aOneSheetOnly )
{
    SCH_REFERENCE_LIST  componentsList;

    // Build the list of components
    if( !aOneSheetOnly )
        Schematic().GetSheets().GetComponents( componentsList );
    else
        GetCurrentSheet().GetComponents( componentsList );

    // Empty schematic does not need annotation
    if( componentsList.GetCount() == 0 )
        return 0;

    return componentsList.CheckAnnotation( aReporter );
}
