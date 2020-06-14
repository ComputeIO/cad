/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <3d_viewer/eda_3d_viewer.h>
#include <kiplatform/ui.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <preview_items/ruler_item.h>
#include <tool/actions.h>
#include <tools/grid_helper.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_viewer_tools.h>
#include <view/view_controls.h>
#include <wx/debug.h>


bool PCB_VIEWER_TOOLS::Init()
{
    // Populate the context menu displayed during the tool (primarily the measure tool)
    auto activeToolCondition =
        [ this ] ( const SELECTION& aSel )
        {
            return !frame()->ToolStackIsEmpty();
        };

    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    ctxMenu.AddSeparator( 1 );

    frame()->AddStandardSubMenus( m_menu );

    return true;
}


void PCB_VIEWER_TOOLS::Reset( RESET_REASON aReason )
{
}


int PCB_VIEWER_TOOLS::Show3DViewer( const TOOL_EVENT& aEvent )
{
    EDA_3D_VIEWER* draw3DFrame = frame()->CreateAndShow3D_Frame();

    if( frame()->IsType( FRAME_FOOTPRINT_VIEWER )
     || frame()->IsType( FRAME_FOOTPRINT_VIEWER_MODAL )
     || frame()->IsType( FRAME_FOOTPRINT_WIZARD ) )
    {
        frame()->Update3DView( true );

        // A stronger version of Raise() which promotes the window to its parent's level.
        KIPLATFORM::UI::ReparentQuasiModal( draw3DFrame );
    }
    return 0;
}


template<class T> void Flip( T& aValue )
{
    aValue = !aValue;
}


int PCB_VIEWER_TOOLS::ZoomAutomatically( const TOOL_EVENT& aEvent )
{
    frame()->SetAutoZoom( !frame()->GetAutoZoom() );

    return 0;
}


int PCB_VIEWER_TOOLS::ShowPadNumbers( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayPadNum );
    frame()->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( auto module : board()->Modules() )
    {
        for( auto pad : module->Pads() )
            view()->Update( pad, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::PadDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayPadFill );
    frame()->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( auto module : board()->Modules() )
    {
        for( auto pad : module->Pads() )
            view()->Update( pad, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::GraphicOutlines( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = displayOptions();

    Flip( opts.m_DisplayGraphicsFill );
    frame()->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( MODULE* module : board()->Modules() )
    {
        for( BOARD_ITEM* item : module->GraphicalItems() )
        {
            if( item->Type() == PCB_MODULE_EDGE_T )
                view()->Update( item, KIGFX::GEOMETRY );
        }
    }

    for( BOARD_ITEM* item : board()->Drawings() )
    {
        KICAD_T t = item->Type();

        if( t == PCB_LINE_T || t == PCB_DIMENSION_T || t == PCB_TARGET_T )
            view()->Update( item, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::TextOutlines( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = displayOptions();

    Flip( opts.m_DisplayTextFill );
    frame()->SetDisplayOptions( opts );
    view()->UpdateDisplayOptions( opts );

    for( MODULE* module : board()->Modules() )
    {
        view()->Update( &module->Reference(), KIGFX::GEOMETRY );
        view()->Update( &module->Value(), KIGFX::GEOMETRY );

        for( BOARD_ITEM* item : module->GraphicalItems() )
        {
            if( item->Type() == PCB_MODULE_TEXT_T )
                view()->Update( item, KIGFX::GEOMETRY );
        }
    }

    for( BOARD_ITEM* item : board()->Drawings() )
    {
        KICAD_T t = item->Type();

        if( t == PCB_TEXT_T || t == PCB_DIMENSION_T )
            view()->Update( item, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::MeasureTool( const TOOL_EVENT& aEvent )
{
    if( IsFootprintFrame() && !frame()->GetModel() )
        return 0;

    auto& view     = *getView();
    auto& controls = *getViewControls();

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );
    Activate();

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;

    EDA_UNITS                  units = frame()->GetUserUnits();
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, units );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    GRID_HELPER grid( m_toolMgr, frame()->GetMagneticItemsSettings() );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );

    while( auto evt = Wait() )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        controls.SetSnapping( !evt->Modifier( MD_ALT ) );
        const VECTOR2I cursorPos = grid.BestSnapAnchor( controls.GetMousePosition(), nullptr );
        controls.ForceCursorPosition(true, cursorPos );

        auto clearRuler =
            [&] ()
            {
                view.SetVisible( &ruler, false );
                controls.SetAutoPan( false );
                controls.CaptureCursor( false );
                originSet = false;
            };

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
                clearRuler();
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }

        else if( evt->IsActivate() )
        {
            if( originSet )
                clearRuler();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }

        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }

        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
        }

        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }

        else if( evt->IsAction( &ACTIONS::toggleUnits )
                 || evt->IsAction( &PCB_ACTIONS::updateUnits ) )
        {
            if( frame()->GetUserUnits() != units )
            {
                units = frame()->GetUserUnits();
                ruler.SwitchUnits();
                view.Update( &ruler, KIGFX::GEOMETRY );
                canvas()->Refresh();
            }
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }

        else
            evt->SetPassEvent();
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );
    return 0;
}


void PCB_VIEWER_TOOLS::setTransitions()
{
    Go( &PCB_VIEWER_TOOLS::Show3DViewer,      ACTIONS::show3DViewer.MakeEvent() );

    // Display modes
    Go( &PCB_VIEWER_TOOLS::ShowPadNumbers,    PCB_ACTIONS::showPadNumbers.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::PadDisplayMode,    PCB_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::GraphicOutlines,   PCB_ACTIONS::graphicsOutlines.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::TextOutlines,      PCB_ACTIONS::textOutlines.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::ZoomAutomatically, PCB_ACTIONS::zoomFootprintAutomatically.MakeEvent() );

    Go( &PCB_VIEWER_TOOLS::MeasureTool,       ACTIONS::measureTool.MakeEvent() );
}
