/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2009-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <bitmaps.h>
#include <netclass.h>
#include <confirm.h>
#include <grid_tricks.h>
#include <panel_setup_netclasses.h>
#include <tool/tool_manager.h>
#include <widgets/wx_grid.h>
#include <kicad_string.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_icon_text_helpers.h>

// PCBNEW columns of netclasses grid
enum {
    GRID_NAME = 0,

    GRID_FIRST_PCBNEW,
    GRID_CLEARANCE = GRID_FIRST_PCBNEW,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
    GRID_DIFF_PAIR_WIDTH,
    GRID_DIFF_PAIR_GAP,

    GRID_FIRST_EESCHEMA,
    GRID_WIREWIDTH = GRID_FIRST_EESCHEMA,
    GRID_BUSWIDTH,
    GRID_SCHEMATIC_COLOR,
    GRID_LINESTYLE,

    GRID_END
};


// These are conceptually constexpr
std::vector<BITMAP_DEF> g_lineStyleIcons;
wxArrayString           g_lineStyleNames;


#define NO_NETCLASS_ASSIGNMENT _( "<unassigned>" )

PANEL_SETUP_NETCLASSES::PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, NETCLASSES* aNetclasses,
                                                const std::vector<wxString>& aCandidateNetNames,
                                                bool aIsEEschema ) :
        PANEL_SETUP_NETCLASSES_BASE( aParent->GetTreebook() ),
        m_Parent( aParent ),
        m_netclasses( aNetclasses ),
        m_candidateNetNames( aCandidateNetNames )
{
    if( g_lineStyleIcons.empty() )
    {
        g_lineStyleIcons.push_back( stroke_solid_xpm );
        g_lineStyleNames.push_back( _( "Solid" ) );
        g_lineStyleIcons.push_back( stroke_dash_xpm );
        g_lineStyleNames.push_back( _( "Dashed" ) );
        g_lineStyleIcons.push_back( stroke_dot_xpm );
        g_lineStyleNames.push_back( _( "Dotted" ) );
        g_lineStyleIcons.push_back( stroke_dashdot_xpm );
        g_lineStyleNames.push_back( _( "Dash-Dot" ) );
    }

    m_netclassesDirty = true;

    // Figure out the smallest the netclass membership pane can ever be so that nothing is cutoff
    // and force it to be that size.
    m_membershipSize = GetSize();
    m_membershipSize.y -= m_netclassesPane->GetSize().y;
    m_membershipSize.x = -1;
    m_membershipPane->SetMinSize( m_membershipSize );
    m_membershipPane->SetMaxSize( m_membershipSize );

    // Prevent Size events from firing before we are ready
    Freeze();
    m_netclassGrid->BeginBatch();
    m_membershipGrid->BeginBatch();

    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];
    // Calculate a min best size to handle longest usual numeric values:
    int min_best_width = m_netclassGrid->GetTextExtent( "555,555555 mils" ).x;

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int min_width =  m_netclassGrid->GetVisibleWidth( i, true, true, false );

        if( i == GRID_LINESTYLE )
            min_best_width *= 1.5;

        m_netclassGrid->SetColMinimalWidth( i, min_width );

        // We use a "best size" >= min_best_width
        m_originalColWidths[ i ] = std::max( min_width, min_best_width );
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
    }

    if( aIsEEschema )
    {
        for( int i = GRID_FIRST_PCBNEW; i < GRID_FIRST_EESCHEMA; ++i )
        {
            m_netclassGrid->HideCol( i );
            m_originalColWidths[ i ] = 0;
        }

        wxGridCellAttr* attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_COLOR_RENDERER() );
        attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( aParent, m_netclassGrid ) );
        m_netclassGrid->SetColAttr( GRID_SCHEMATIC_COLOR, attr );

        attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_lineStyleIcons, g_lineStyleNames ) );
        attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_lineStyleIcons, g_lineStyleNames ) );
        m_netclassGrid->SetColAttr( GRID_LINESTYLE, attr );
    }
    else
    {
        for( int i = GRID_FIRST_EESCHEMA; i < GRID_END; ++i )
        {
            m_netclassGrid->HideCol( i );
            m_originalColWidths[ i ] = 0;
        }
    }

    // Be sure the column labels are readable
    m_netclassGrid->EnsureColLabelsVisible();

    // Membership combobox editors require a bit more room, so increase the row size of
    // all our grids for consistency
    m_netclassGrid->SetDefaultRowSize( m_netclassGrid->GetDefaultRowSize() + 4 );
    m_membershipGrid->SetDefaultRowSize( m_membershipGrid->GetDefaultRowSize() + 4 );

    m_netclassGrid->PushEventHandler( new GRID_TRICKS( m_netclassGrid ) );
    m_membershipGrid->PushEventHandler( new GRID_TRICKS( m_membershipGrid ) );

    m_netclassGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_membershipGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Set up the net name column of the netclass membership grid to read-only
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_membershipGrid->SetColAttr( 0, attr );

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_removeButton->SetBitmap( KiBitmap( trash_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_netclassGrid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ), NULL, this );

    m_netclassGrid->EndBatch();
    m_membershipGrid->EndBatch();
    Thaw();
}


PANEL_SETUP_NETCLASSES::~PANEL_SETUP_NETCLASSES()
{
    delete [] m_originalColWidths;

    // Delete the GRID_TRICKS.
    m_netclassGrid->PopEventHandler( true );
    m_membershipGrid->PopEventHandler( true );

    m_netclassGrid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging ), NULL, this );
}


static void netclassToGridRow( EDA_UNITS aUnits, wxGrid* aGrid, int aRow, const NETCLASSPTR& nc )
{
    aGrid->SetCellValue( aRow, GRID_NAME, nc->GetName() );

#define SET_MILS_CELL( col, val ) \
    aGrid->SetCellValue( aRow, col, StringFromValue( aUnits, val, true, true ) )

    SET_MILS_CELL( GRID_CLEARANCE, nc->GetClearance() );
    SET_MILS_CELL( GRID_TRACKSIZE, nc->GetTrackWidth() );
    SET_MILS_CELL( GRID_VIASIZE, nc->GetViaDiameter() );
    SET_MILS_CELL( GRID_VIADRILL, nc->GetViaDrill() );
    SET_MILS_CELL( GRID_uVIASIZE, nc->GetuViaDiameter() );
    SET_MILS_CELL( GRID_uVIADRILL, nc->GetuViaDrill() );
    SET_MILS_CELL( GRID_DIFF_PAIR_WIDTH, nc->GetDiffPairWidth() );
    SET_MILS_CELL( GRID_DIFF_PAIR_GAP, nc->GetDiffPairGap() );

    SET_MILS_CELL( GRID_WIREWIDTH, nc->GetWireWidth() );
    SET_MILS_CELL( GRID_BUSWIDTH, nc->GetBusWidth() );

    wxString colorAsString = nc->GetSchematicColor().ToWxString( wxC2S_CSS_SYNTAX );
    aGrid->SetCellValue( aRow, GRID_SCHEMATIC_COLOR, colorAsString );
    aGrid->SetCellValue( aRow, GRID_LINESTYLE, g_lineStyleNames[ nc->GetLineStyle() ] );
}


bool PANEL_SETUP_NETCLASSES::TransferDataToWindow()
{
    std::map<wxString, wxString> netToNetclassMap;
    std::map<wxString, wxString> staleNetMap;

    for( const wxString& candidate : m_candidateNetNames )
        netToNetclassMap[ candidate ] = wxEmptyString;

    if( m_netclassGrid->GetNumberRows() )
        m_netclassGrid->DeleteRows( 0, m_netclassGrid->GetNumberRows() );

    m_netclassGrid->AppendRows((int) m_netclasses->GetCount() + 1 ); // + 1 for default netclass

    // enter the Default NETCLASS.
    netclassToGridRow( m_Parent->GetUserUnits(), m_netclassGrid, 0, m_netclasses->GetDefault() );

    // make the Default NETCLASS name read-only
    wxGridCellAttr* cellAttr = m_netclassGrid->GetOrCreateCellAttr( 0, GRID_NAME );
    cellAttr->SetReadOnly();
    cellAttr->DecRef();

    // enter other netclasses
    int row = 1;

    for( NETCLASSES::iterator i = m_netclasses->begin(); i != m_netclasses->end(); ++i, ++row )
    {
        NETCLASSPTR netclass = i->second;

        netclassToGridRow( m_Parent->GetUserUnits(), m_netclassGrid, row, netclass );

        for( const wxString& net : *netclass )
        {
            // While we currently only store shortNames as members, legacy versions stored
            // fully-qualified names so we don't know which kind we're going to find.
            wxString shortName = net.AfterLast( '/' );

            if( netToNetclassMap.count( shortName ) )
                netToNetclassMap[ shortName ] = i->second->GetName();
            else
                staleNetMap[ shortName ] = i->second->GetName();
        }
    }

    if( m_membershipGrid->GetNumberRows() )
        m_membershipGrid->DeleteRows( 0, m_membershipGrid->GetNumberRows() );

    // add currently-assigned and candidate netnames to membership lists
    for( const std::pair<const wxString, wxString>& ii : netToNetclassMap )
        addNet( UnescapeString( ii.first ), ii.second, false );

    for( const std::pair<const wxString, wxString>& ii : staleNetMap )
        addNet( UnescapeString( ii.first ), ii.second, true );

    return true;
}


void PANEL_SETUP_NETCLASSES::addNet( const wxString& netName, const wxString& netclass,
                                     bool aStale )
{
    int i = m_membershipGrid->GetNumberRows();

    m_membershipGrid->AppendRows( 1 );

    m_membershipGrid->SetCellValue( i, 0, netName );

    if( aStale )
    {
        wxColour color = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
        m_membershipGrid->SetCellTextColour( i, 0, color );
    }

    m_membershipGrid->SetCellValue( i, 1, netclass );
}


/*
 * Populates drop-downs with the list of net classes
 */
void PANEL_SETUP_NETCLASSES::rebuildNetclassDropdowns()
{
    m_membershipGrid->CommitPendingChanges( true );

    wxArrayString netclassNames;

    netclassNames.push_back( NO_NETCLASS_ASSIGNMENT );

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( ii, GRID_NAME );
        if( !netclassName.IsEmpty() )
            netclassNames.push_back( netclassName );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( netclassNames ) );
    m_membershipGrid->SetColAttr( 1, attr );

    m_assignNetClass->Set( netclassNames );

    netclassNames.Insert( wxEmptyString, 0 );
    m_netClassFilter->Set( netclassNames );
}


static void gridRowToNetclass( EDA_UNITS aUnits, wxGrid* grid, int row, const NETCLASSPTR& nc )
{
    nc->SetName( grid->GetCellValue( row, GRID_NAME ) );

#define MYCELL( col )   \
    ValueFromString( aUnits, grid->GetCellValue( row, col ), true )

    nc->SetClearance( MYCELL( GRID_CLEARANCE ) );
    nc->SetTrackWidth( MYCELL( GRID_TRACKSIZE ) );
    nc->SetViaDiameter( MYCELL( GRID_VIASIZE ) );
    nc->SetViaDrill( MYCELL( GRID_VIADRILL ) );
    nc->SetuViaDiameter( MYCELL( GRID_uVIASIZE ) );
    nc->SetuViaDrill( MYCELL( GRID_uVIADRILL ) );
    nc->SetDiffPairWidth( MYCELL( GRID_DIFF_PAIR_WIDTH ) );
    nc->SetDiffPairGap( MYCELL( GRID_DIFF_PAIR_GAP ) );

    nc->SetWireWidth( MYCELL( GRID_WIREWIDTH ) );
    nc->SetBusWidth( MYCELL( GRID_BUSWIDTH ) );

    nc->SetSchematicColor( wxColour( grid->GetCellValue( row, GRID_SCHEMATIC_COLOR ) ) );
    nc->SetLineStyle( g_lineStyleNames.Index( grid->GetCellValue( row, GRID_LINESTYLE ) ) );
}


bool PANEL_SETUP_NETCLASSES::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    m_netclasses->Clear();

    // Copy the default NetClass:
    gridRowToNetclass( m_Parent->GetUserUnits(), m_netclassGrid, 0, m_netclasses->GetDefault() );

    // Copy other NetClasses:
    for( int row = 1; row < m_netclassGrid->GetNumberRows();  ++row )
    {
        NETCLASSPTR nc = std::make_shared<NETCLASS>( m_netclassGrid->GetCellValue( row, GRID_NAME ) );

        if( m_netclasses->Add( nc ) )
            gridRowToNetclass( m_Parent->GetUserUnits(), m_netclassGrid, row, nc );
    }

    // Now read all nets and push them in the corresponding netclass net buffer
    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        const wxString& netname = m_membershipGrid->GetCellValue( row, 0 );
        const wxString& classname = m_membershipGrid->GetCellValue( row, 1 );

        if( classname != NO_NETCLASS_ASSIGNMENT )
        {
            const NETCLASSPTR& nc = m_netclasses->Find( classname );

            if( nc )
                nc->Add( EscapeString( netname, CTX_NETNAME ) );
        }
    }

    return true;
}


bool PANEL_SETUP_NETCLASSES::validateNetclassName( int aRow, wxString aName, bool focusFirst )
{
    aName.Trim( true );
    aName.Trim( false );

    if( aName.IsEmpty() )
    {
        wxString msg =  _( "Netclass must have a name." );
        m_Parent->SetError( msg, this, m_netclassGrid, aRow, GRID_NAME );
        return false;
    }

    for( int ii = 0; ii < m_netclassGrid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && m_netclassGrid->GetCellValue( ii, GRID_NAME ).CmpNoCase( aName ) == 0 )
        {
            wxString msg = _( "Netclass name already in use." );
            m_Parent->SetError( msg, this, m_netclassGrid, focusFirst ? aRow : ii, GRID_NAME );
            return false;
        }
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::OnNetclassGridCellChanging( wxGridEvent& event )
{
    if( event.GetCol() == GRID_NAME )
    {
        if( validateNetclassName( event.GetRow(), event.GetString() ) )
        {
            wxString oldName = m_netclassGrid->GetCellValue( event.GetRow(), GRID_NAME );
            wxString newName = event.GetString();

            for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
            {
                if( m_membershipGrid->GetCellValue( row, 1 ) == oldName )
                    m_membershipGrid->SetCellValue( row, 1, newName );
            }

            m_netclassesDirty = true;
        }
        else
            event.Veto();
    }
}


void PANEL_SETUP_NETCLASSES::OnAddNetclassClick( wxCommandEvent& event )
{
    if( !m_netclassGrid->CommitPendingChanges() )
        return;

    int row = m_netclassGrid->GetNumberRows();
    m_netclassGrid->AppendRows();

    // Copy values of the default class:
    for( int col = 1; col < m_netclassGrid->GetNumberCols(); col++ )
        m_netclassGrid->SetCellValue( row, col, m_netclassGrid->GetCellValue( 0, col ) );

    m_netclassGrid->MakeCellVisible( row, 0 );
    m_netclassGrid->SetGridCursor( row, 0 );

    m_netclassGrid->EnableCellEditControl( true );
    m_netclassGrid->ShowCellEditControl();

    m_netclassesDirty = true;
}


void PANEL_SETUP_NETCLASSES::OnRemoveNetclassClick( wxCommandEvent& event )
{
    if( !m_netclassGrid->CommitPendingChanges() )
        return;

    int curRow = m_netclassGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;
    else if( curRow == 0 )
    {
        DisplayErrorMessage( this, _( "The default net class is required." ) );
        return;
    }

    // reset the net class to default for members of the removed class
    wxString classname = m_netclassGrid->GetCellValue( curRow, GRID_NAME );

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        if( m_membershipGrid->GetCellValue( row, 1 ) == classname )
            m_membershipGrid->SetCellValue( row, 1, NETCLASS::Default );
    }

    m_netclassGrid->DeleteRows( curRow, 1 );

    m_netclassGrid->MakeCellVisible( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );
    m_netclassGrid->SetGridCursor( std::max( 0, curRow-1 ), m_netclassGrid->GetGridCursorCol() );

    m_netclassesDirty = true;
}


void PANEL_SETUP_NETCLASSES::AdjustNetclassGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_netclassGrid->GetSize().x - m_netclassGrid->GetClientSize().x );

    for( int i = 1; i < m_netclassGrid->GetNumberCols(); i++ )
    {
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
        aWidth -= m_originalColWidths[ i ];
    }

    m_netclassGrid->SetColSize( 0, std::max( aWidth - 2, m_originalColWidths[ 0 ] ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeNetclassGrid( wxSizeEvent& event )
{
    AdjustNetclassGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::OnMembershipKillFocus( wxFocusEvent& event )
{
    m_membershipGrid->CommitPendingChanges();
}


void PANEL_SETUP_NETCLASSES::AdjustMembershipGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_membershipGrid->GetSize().x - m_membershipGrid->GetClientSize().x );

    // Set className column width to original className width from netclasses grid
    int classNameWidth = m_originalColWidths[ 0 ];
    m_membershipGrid->SetColSize( 1, m_originalColWidths[ 0 ] );
    m_membershipGrid->SetColSize( 0, std::max( aWidth - classNameWidth, classNameWidth ) );
}


void PANEL_SETUP_NETCLASSES::OnSizeMembershipGrid( wxSizeEvent& event )
{
    AdjustMembershipGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_SETUP_NETCLASSES::doApplyFilters( bool aShowAll )
{
    if( !m_membershipGrid->CommitPendingChanges() )
        return;

    wxString netClassFilter = m_netClassFilter->GetStringSelection();
    wxString netFilter = m_netNameFilter->GetValue().MakeLower();

    if( !netFilter.IsEmpty() )
        netFilter = wxT( "*" ) + netFilter + wxT( "*" );

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        wxString net = m_membershipGrid->GetCellValue( row, 0 );
        wxString netClass = m_membershipGrid->GetCellValue( row, 1 );
        bool show = true;

        if( !aShowAll )
        {
            if( !netFilter.IsEmpty() && !net.MakeLower().Matches( netFilter ) )
                show = false;

            if( !netClassFilter.IsEmpty() && netClass != netClassFilter )
                show = false;
        }

        if( show )
            m_membershipGrid->ShowRow( row );
        else
            m_membershipGrid->HideRow( row );
    }
}


void PANEL_SETUP_NETCLASSES::doAssignments( bool aAssignAll )
{
    if( !m_membershipGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_membershipGrid->GetSelectedRows();

    for( int row = 0; row < m_membershipGrid->GetNumberRows(); ++row )
    {
        if( !m_membershipGrid->IsRowShown( row ) )
            continue;

        if( !aAssignAll && selectedRows.Index( row ) == wxNOT_FOUND )
            continue;

        m_membershipGrid->SetCellValue( row, 1, m_assignNetClass->GetStringSelection() );
    }
}


void PANEL_SETUP_NETCLASSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_netclassesDirty )
    {
        rebuildNetclassDropdowns();
        m_netclassesDirty = false;
    }

    // Recompute the desired size for the two content panes. We cannot leave this sizing to
    // wxWidgets because it wants to shrink the membership panel to an unusable size when the
    // netlist panel grows, and also it introduces undesired artifacts when the window is resized
    // and the panes can grow/shrink.
    wxSize netclassSize = GetClientSize();
    netclassSize.y -= m_membershipSize.y;

    // Modify m_netclassesPane size only if needed, because calling Layout() has the annoying
    // effect of closing any open wxChoice dropdowns.  So it cannot blindly called inside each
    // wxUpdateUIEvent event, at least on Windows + wxWidgets 3.0 (not an issue with 3.1.1).
    if( netclassSize.y != m_netclassesPane->GetSize().y )
    {
        m_netclassesPane->SetMinSize( netclassSize );
        m_netclassesPane->SetMaxSize( netclassSize );
        Layout();
    }
}


bool PANEL_SETUP_NETCLASSES::validateData()
{
    if( !m_netclassGrid->CommitPendingChanges() || !m_membershipGrid->CommitPendingChanges() )
        return false;

    wxString msg;

    // Test net class parameters.
    for( int row = 0; row < m_netclassGrid->GetNumberRows(); row++ )
    {
        wxString netclassName = m_netclassGrid->GetCellValue( row, GRID_NAME );
        netclassName.Trim( true );
        netclassName.Trim( false );

        if( !validateNetclassName( row, netclassName, false ) )
            return false;
    }

    return true;
}


void PANEL_SETUP_NETCLASSES::ImportSettingsFrom( NETCLASSES* aNetclasses )
{
    NETCLASSES* savedSettings = m_netclasses;

    m_netclasses = aNetclasses;
    TransferDataToWindow();

    rebuildNetclassDropdowns();

    m_netclassGrid->ForceRefresh();
    m_membershipGrid->ForceRefresh();

    m_netclasses = savedSettings;
}


