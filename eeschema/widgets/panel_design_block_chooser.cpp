/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <design_block.h>
#include <design_block_lib_table.h>
#include <panel_design_block_chooser.h>
#include <kiface_base.h>
#include <sch_base_frame.h>
#include <project_sch.h>
#include <widgets/lib_tree.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <eeschema_settings.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/wxhtml.h>
#include <wx/msgdlg.h>
#include <widgets/wx_progress_reporters.h>


wxString PANEL_DESIGN_BLOCK_CHOOSER::g_design_blockSearchString;
wxString PANEL_DESIGN_BLOCK_CHOOSER::g_powerSearchString;


PANEL_DESIGN_BLOCK_CHOOSER::PANEL_DESIGN_BLOCK_CHOOSER( SCH_BASE_FRAME* aFrame, wxWindow* aParent,
                                                        std::vector<LIB_ID>&  aHistoryList,
                                                        std::function<void()> aSelectHandler ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_vsplitter( nullptr ),
        m_tree( nullptr ),
        m_details( nullptr ),
        m_frame( aFrame ),
        m_selectHandler( std::move( aSelectHandler ) ),
        m_historyList( aHistoryList )
{
    DESIGN_BLOCK_LIB_TABLE*   libs = m_frame->Prj().DesignBlockLibs();
    COMMON_SETTINGS::SESSION& session = Pgm().GetCommonSettings()->m_Session;
    PROJECT_FILE&             project = m_frame->Prj().GetProjectFile();

    // Make sure settings are loaded before we start running multi-threaded design block loaders
    Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    // Load design block files:
    WX_PROGRESS_REPORTER* progressReporter =
            new WX_PROGRESS_REPORTER( aParent, _( "Loading Design Block Libraries" ), 3 );
    GDesignBlockList.ReadDesignBlockFiles( libs, nullptr, progressReporter );

    // Force immediate deletion of the WX_PROGRESS_REPORTER.  Do not use Destroy(), or use
    // Destroy() followed by wxSafeYield() because on Windows, APP_PROGRESS_DIALOG and
    // WX_PROGRESS_REPORTER have some side effects on the event loop manager.  For instance, a
    // subsequent call to ShowModal() or ShowQuasiModal() for a dialog following the use of a
    // WX_PROGRESS_REPORTER results in incorrect modal or quasi modal behavior.
    delete progressReporter;

    if( GDesignBlockList.GetErrorCount() )
        wxMessageBox("Error loading design blocks", "Error", wxICON_ERROR);
        //GDesignBlockList.DisplayErrors( aParent );

    m_adapter = DESIGN_BLOCK_TREE_MODEL_ADAPTER::Create( m_frame, libs );

    // -------------------------------------------------------------------------------------
    // Construct the actual panel
    //

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_vsplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );


    // Avoid the splitter window being assigned as the parent to additional windows.
    m_vsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );

    wxPanel*    treePanel = new wxPanel( m_vsplitter );
    wxBoxSizer* treeSizer = new wxBoxSizer( wxVERTICAL );
    treePanel->SetSizer( treeSizer );

    wxPanel*    detailsPanel = new wxPanel( m_vsplitter );
    wxBoxSizer* detailsSizer = new wxBoxSizer( wxVERTICAL );
    detailsPanel->SetSizer( detailsSizer );

    m_details = new HTML_WINDOW( detailsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize );
    detailsSizer->Add( m_details, 1, wxEXPAND, 5 );
    detailsPanel->Layout();
    detailsSizer->Fit( detailsPanel );

    m_vsplitter->SetSashGravity( 0.5 );
    m_vsplitter->SetMinimumPaneSize( 20 );
    m_vsplitter->SplitHorizontally( treePanel, detailsPanel );

    sizer->Add( m_vsplitter, 1, wxEXPAND, 5 );


    m_tree = new LIB_TREE( treePanel, wxT( "design_blocks" ), libs, m_adapter,
                           LIB_TREE::FLAGS::ALL_WIDGETS, m_details );

    treeSizer->Add( m_tree, 1, wxEXPAND, 5 );
    treePanel->Layout();
    treeSizer->Fit( treePanel );

    RefreshLibs();
    m_adapter->FinishTreeInitialization();

    m_tree->SetSearchString( g_design_blockSearchString );

    m_dbl_click_timer = new wxTimer( this );
    m_open_libs_timer = new wxTimer( this );

    SetSizer( sizer );

    Layout();

    Bind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer, this,
          m_dbl_click_timer->GetId() );
    Bind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onOpenLibsTimer, this,
          m_open_libs_timer->GetId() );
    Bind( EVT_LIBITEM_SELECTED, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected, this );
    Bind( EVT_LIBITEM_CHOSEN, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen, this );
    Bind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, this );

    if( m_details )
    {
        m_details->Connect( wxEVT_CHAR_HOOK,
                            wxKeyEventHandler( PANEL_DESIGN_BLOCK_CHOOSER::OnDetailsCharHook ),
                            nullptr, this );
    }

    // Open the user's previously opened libraries on timer expiration.
    // This is done on a timer because we need a gross hack to keep GTK from garbling the
    // display. Must be longer than the search debounce timer.
    m_open_libs_timer->StartOnce( 300 );
}


PANEL_DESIGN_BLOCK_CHOOSER::~PANEL_DESIGN_BLOCK_CHOOSER()
{
    Unbind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer, this );
    Unbind( EVT_LIBITEM_SELECTED, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected, this );
    Unbind( EVT_LIBITEM_CHOSEN, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen, this );
    Unbind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, this );

    // Stop the timer during destruction early to avoid potential race conditions (that do happen)
    m_dbl_click_timer->Stop();
    m_open_libs_timer->Stop();
    delete m_dbl_click_timer;
    delete m_open_libs_timer;

    g_design_blockSearchString = m_tree->GetSearchString();


    if( m_details )
    {
        m_details->Disconnect( wxEVT_CHAR_HOOK,
                               wxKeyEventHandler( PANEL_DESIGN_BLOCK_CHOOSER::OnDetailsCharHook ),
                               nullptr, this );
    }

    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        // Save any changes to column widths, etc.
        m_adapter->SaveSettings();

        cfg->m_DesignBlockChooserPanel.width = GetParent()->GetSize().x;
        cfg->m_DesignBlockChooserPanel.height = GetParent()->GetSize().y;

        if( m_vsplitter )
            cfg->m_DesignBlockChooserPanel.sash_pos_v = m_vsplitter->GetSashPosition();

        cfg->m_DesignBlockChooserPanel.sort_mode = m_tree->GetSortMode();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::OnChar( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        wxObject* eventSource = aEvent.GetEventObject();

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
        {
            // First escape cancels search string value
            if( textCtrl->GetValue() == m_tree->GetSearchString()
                && !m_tree->GetSearchString().IsEmpty() )
            {
                m_tree->SetSearchString( wxEmptyString );
                return;
            }
        }
    }
    else
    {
        aEvent.Skip();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::FinishSetup()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        auto horizPixelsFromDU =
                [&]( int x ) -> int
                {
                    wxSize sz( x, 0 );
                    return GetParent()->ConvertDialogToPixels( sz ).x;
                };

        EESCHEMA_SETTINGS::PANEL_DESIGN_BLOCK_CHOOSER& panelCfg = cfg->m_DesignBlockChooserPanel;

        int w = panelCfg.width > 40 ? panelCfg.width : horizPixelsFromDU( 440 );
        int h = panelCfg.height > 40 ? panelCfg.height : horizPixelsFromDU( 340 );

        GetParent()->SetSize( wxSize( w, h ) );
        GetParent()->Layout();

        // We specify the width of the right window (m_design_block_view_panel), because specify
        // the width of the left window does not work as expected when SetSashGravity() is called

        if( panelCfg.sash_pos_h < 0 )
            panelCfg.sash_pos_h = horizPixelsFromDU( 220 );

        if( panelCfg.sash_pos_v < 0 )
            panelCfg.sash_pos_v = horizPixelsFromDU( 230 );

        if( m_vsplitter )
            m_vsplitter->SetSashPosition( panelCfg.sash_pos_v );

        m_adapter->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) panelCfg.sort_mode );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::OnDetailsCharHook( wxKeyEvent& e )
{
    if( m_details && e.GetKeyCode() == 'C' && e.ControlDown() && !e.AltDown() && !e.ShiftDown()
        && !e.MetaDown() )
    {
        wxString  txt = m_details->SelectionToText();
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( txt ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }
    else
    {
        e.Skip();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::RefreshLibs()
{
    // Unselect before syncing to avoid null reference in the adapter
    // if a selected item is removed during the sync
    m_tree->Unselect();

    DESIGN_BLOCK_TREE_MODEL_ADAPTER* adapter =
            static_cast<DESIGN_BLOCK_TREE_MODEL_ADAPTER*>( m_adapter.get() );

    // Clear all existing libraries then re-add
    adapter->ClearLibraries();

    std::vector<DESIGN_BLOCK>   history_list_storage;
    std::vector<LIB_TREE_ITEM*> history_list;

    // Lambda to encapsulate the common logic
    auto processList =
            [&]( const std::vector<LIB_ID>& inputList,
                 std::vector<DESIGN_BLOCK>&          storageList,
                 std::vector<LIB_TREE_ITEM*>&      resultList )
            {
                storageList.reserve( inputList.size() );

                for( const LIB_ID& i : inputList )
                {
                    DESIGN_BLOCK* design_block = m_frame->GetDesignBlock( i );

                    if( design_block )
                    {
                        storageList.emplace_back( *design_block );
                    }
                }
            };

    processList( m_historyList, history_list_storage, history_list );

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                           history_list, false, true );

    if( !m_historyList.empty() )
        adapter->SetPreselectNode( m_historyList[0], 0 );

    adapter->AddLibraries( m_frame );


    m_tree->Regenerate( true );
}


void PANEL_DESIGN_BLOCK_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );
}


LIB_ID PANEL_DESIGN_BLOCK_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_tree->GetSelectedLibId( aUnit );
}


void PANEL_DESIGN_BLOCK_CHOOSER::SelectLibId( const LIB_ID& aLibId )
{
    m_tree->CenterLibId( aLibId );
    m_tree->SelectLibId( aLibId );
}


void PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer( wxTimerEvent& aEvent )
{
    // Hack because of eaten MouseUp event. See PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen
    // for the beginning of this spaghetti noodle.

    wxMouseState state = wxGetMouseState();

    if( state.LeftIsDown() )
    {
        // Mouse hasn't been raised yet, so fire the timer again. Otherwise the
        // purpose of this timer is defeated.
        m_dbl_click_timer->StartOnce( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
    }
    else
    {
        m_selectHandler();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::onOpenLibsTimer( wxTimerEvent& aEvent )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
        m_adapter->OpenLibs( cfg->m_LibTree.open_libs );
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected( wxCommandEvent& aEvent )
{
    LIB_TREE_NODE* node = m_tree->GetCurrentTreeNode();
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen( wxCommandEvent& aEvent )
{
    if( m_tree->GetSelectedLibId().IsValid() )
    {
        // Got a selection. We can't just end the modal dialog here, because wx leaks some events
        // back to the parent window (in particular, the MouseUp following a double click).
        //
        // NOW, here's where it gets really fun. wxTreeListCtrl eats MouseUp.  This isn't really
        // feasible to bypass without a fully custom wxDataViewCtrl implementation, and even then
        // might not be fully possible (docs are vague). To get around this, we use a one-shot
        // timer to schedule the dialog close.
        //
        // See PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer for the other end of this spaghetti noodle.
        m_dbl_click_timer->StartOnce( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
    }
}
