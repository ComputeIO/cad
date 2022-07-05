/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "sim_plot_colors.h"
#include "sim_plot_panel.h"
#include "sim_plot_frame.h"

#include <algorithm>
#include <vector>
#include <limits>


SIM_PLOT_AXIS_Y::SIM_PLOT_AXIS_Y( SIM_QUANTITY aQ, const wxString& name, const wxString& aUnit,
                                  int flags, bool ticks ) :
        mpScaleY( name, flags, ticks )
{
    m_quantity = aQ;
    m_nameFlags = flags;
    m_unit = aUnit;
}

SIM_PLOT_AXIS_X::SIM_PLOT_AXIS_X( SIM_QUANTITY aQ, const wxString& name, const wxString& aUnit,
                                  int flags, bool ticks ) :
        mpScaleX( name, flags, ticks )
{
    m_quantity = aQ;
    m_nameFlags = flags;
    m_unit = aUnit;
}


static wxString formatFloat( double x, int nDigits )
{
    wxString rv, fmt;

    if( nDigits )
    {
        fmt.Printf( "%%.0%df", nDigits );
    }
    else
    {
        fmt = wxT( "%.0f" );
    }

    rv.Printf( fmt, x );

    return rv;
}


static void getSISuffix( double x, const wxString& unit, int& power, wxString& suffix )
{
    const int n_powers = 11;

    const struct
    {
        double exponent;
        char suffix;
    } powers[] =
    {
        { -18, 'a' },
        { -15, 'f' },
        { -12, 'p' },
        { -9,  'n' },
        { -6,  'u' },
        { -3,  'm' },
        { 0,   0   },
        { 3,   'k' },
        { 6,   'M' },
        { 9,   'G' },
        { 12,  'T' },
        { 14,  'P' }
    };

    power = 0;
    suffix = unit;

    if( x == 0.0 )
        return;

    for( int i = 0; i < n_powers - 1; i++ )
    {
        double r_cur = pow( 10, powers[i].exponent );

        if( fabs( x ) >= r_cur && fabs( x ) < r_cur * 1000.0 )
        {
            power = powers[i].exponent;

            if( powers[i].suffix )
                suffix = wxString( powers[i].suffix ) + unit;
            else
                suffix = unit;

            return;
        }
    }
}


static int countDecimalDigits( double x, int maxDigits )
{
    if( std::isnan( x ) )
    {
        // avoid trying to count the decimals of NaN
        return 0;
    }

    int64_t k = (int)( ( x - floor( x ) ) * pow( 10.0, (double) maxDigits ) );
    int n = 0;

    while( k && ( ( k % 10LL ) == 0LL || ( k % 10LL ) == 9LL ) )
    {
        k /= 10LL;
    }

    n = 0;

    while( k != 0LL )
    {
        n++;
        k /= 10LL;
    }

    return n;
}


void mpScaleBase::formatLabels()
{
    wxString suffix;
    int      power;

    if( m_log )
    {
        int power;

        for( auto& l : TickLabels() )
        {
            getSISuffix( l.pos, m_unit, power, suffix );
            double sf = pow( 10.0, power );
            int    k = countDecimalDigits( l.pos / sf, 3 );

            l.label = formatFloat( l.pos / sf, k ) + suffix;
            l.visible = true;
        }
    }
    else
    {
        double maxVis = AbsVisibleMaxValue();

        int digits = 0;
        int constexpr DIGITS = 3;

        getSISuffix( maxVis, m_unit, power, suffix );

        double sf = pow( 10.0, power );

        for( auto& l : TickLabels() )
        {
            int k = countDecimalDigits( l.pos / sf, DIGITS );

            digits = std::max( digits, k );
        }

        for( auto& l : TickLabels() )
        {
            l.label = formatFloat( l.pos / sf, digits ) + suffix;
            l.visible = true;
        }
    }
}

void CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_window )
        m_window = &aWindow;

    if( !m_visible )
        return;

    const auto& dataX = m_trace->GetDataX();
    const auto& dataY = m_trace->GetDataY();

    if( dataX.size() <= 1 )
        return;

    if( m_updateRequired )
    {
        m_coords.x = m_trace->s2x( aWindow.p2x( m_dim.x ) );

        // Find the closest point coordinates
        auto maxXIt = std::upper_bound( dataX.begin(), dataX.end(), m_coords.x );
        int maxIdx = maxXIt - dataX.begin();
        int minIdx = maxIdx - 1;

        // Out of bounds checks
        if( minIdx < 0 )
        {
            minIdx = 0;
            maxIdx = 1;
            m_coords.x = dataX[0];
        }
        else if( maxIdx >= (int) dataX.size() )
        {
            maxIdx = dataX.size() - 1;
            minIdx = maxIdx - 1;
            m_coords.x = dataX[maxIdx];
        }

        const double leftX = dataX[minIdx];
        const double rightX = dataX[maxIdx];
        const double leftY = dataY[minIdx];
        const double rightY = dataY[maxIdx];

        // Linear interpolation
        m_coords.y = leftY + ( rightY - leftY ) / ( rightX - leftX ) * ( m_coords.x - leftX );
        m_updateRequired = false;

        // Notify the parent window about the changes
        wxQueueEvent( aWindow.GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
    else
    {
        m_updateRef = true;
    }

    if( m_updateRef )
    {
        UpdateReference();
        m_updateRef = false;
    }

    // Line length in horizontal and vertical dimensions
    const wxPoint cursorPos( aWindow.x2p( m_trace->x2s( m_coords.x ) ),
                             aWindow.y2p( m_trace->y2s( m_coords.y ) ) );

    wxCoord leftPx   = m_drawOutsideMargins ? 0 : aWindow.GetMarginLeft();
    wxCoord rightPx  = m_drawOutsideMargins ? aWindow.GetScrX() :
                                              aWindow.GetScrX() - aWindow.GetMarginRight();
    wxCoord topPx    = m_drawOutsideMargins ? 0 : aWindow.GetMarginTop();
    wxCoord bottomPx = m_drawOutsideMargins ? aWindow.GetScrY() :
                                              aWindow.GetScrY() - aWindow.GetMarginBottom();

    wxPen pen = GetPen();
    pen.SetStyle( m_continuous ? wxPENSTYLE_SOLID : wxPENSTYLE_LONG_DASH );
    aDC.SetPen( pen );

    if( topPx < cursorPos.y && cursorPos.y < bottomPx )
        aDC.DrawLine( leftPx, cursorPos.y, rightPx, cursorPos.y );

    if( leftPx < cursorPos.x && cursorPos.x < rightPx )
        aDC.DrawLine( cursorPos.x, topPx, cursorPos.x, bottomPx );
}


bool CURSOR::Inside( wxPoint& aPoint )
{
    if( !m_window )
        return false;

    return ( std::abs( (double) aPoint.x -
                       m_window->x2p( m_trace->x2s( m_coords.x ) ) ) <= DRAG_MARGIN )
        || ( std::abs( (double) aPoint.y -
                       m_window->y2p( m_trace->y2s( m_coords.y ) ) ) <= DRAG_MARGIN );
}


void CURSOR::UpdateReference()
{
    if( !m_window )
        return;

    m_reference.x = m_window->x2p( m_trace->x2s( m_coords.x ) );
    m_reference.y = m_window->y2p( m_trace->y2s( m_coords.y ) );
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( const wxString& aCommand, wxWindow* parent,
                                SIM_PLOT_FRAME* aMainFrame, wxWindowID id, const wxPoint& pos,
                                const wxSize& size, long style, const wxString& name )
    : SIM_PANEL_BASE( aCommand, parent, id, pos, size, style, name ),
      m_axis_x( nullptr ),
      m_dotted_cp( false ),
      m_masterFrame( aMainFrame )
{
    m_sizer   = new wxBoxSizer( wxVERTICAL );
    m_plotWin = new mpWindow( this, wxID_ANY, pos, size, style );

    m_plotWin->LimitView( true );
    m_plotWin->SetMargins( 50, 80, 50, 80 );

    UpdatePlotColors();

    updateAxes();

    // a mpInfoLegend displays le name of traces on the left top panel corner:
    m_legend = new mpInfoLegend( wxRect( 0, 40, 200, 40 ), wxTRANSPARENT_BRUSH );
    m_legend->SetVisible( false );
    m_plotWin->AddLayer( m_legend );

    m_plotWin->EnableDoubleBuffer( true );
    m_plotWin->UpdateAll();

    m_sizer->Add( m_plotWin, 1, wxALL | wxEXPAND, 1 );
    SetSizer( m_sizer );

    m_plotWin->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( SIM_PLOT_PANEL::OnRightClick ), NULL,
                        this );
}

void SIM_PLOT_PANEL::OnRightClick( wxMouseEvent& event )
{
    int clickedX = event.GetX();
    int clickedY = event.GetY();

    int topM = m_plotWin->GetMarginTop();
    int rightM = m_plotWin->GetMarginRight();
    int leftM = m_plotWin->GetMarginLeft();
    int botM = m_plotWin->GetMarginBottom();

    int sizeX = m_plotWin->GetScrX();
    int sizeY = m_plotWin->GetScrY();

    // @TODO implement function that returns the bouding boxe of axes...

    if( ( clickedX > leftM ) && ( clickedX < sizeX - rightM ) && ( clickedY > sizeY - botM ) )
    {
        m_axis_x->m_log = !m_axis_x->m_log;
        m_plotWin->UpdateAll();
    }
    else
    {
        m_plotWin->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( SIM_PLOT_PANEL::OnRightClick ),
                            NULL, this );
        m_plotWin->PopupMenu( m_plotWin->GetPopupMenu(), event.GetX(), event.GetY() );
    }
}

SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
    m_plotWin->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( SIM_PLOT_PANEL::OnRightClick ),
                           NULL, this );
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}

SIM_PLOT_AXIS_Y* SIM_PLOT_PANEL::GetAxeY( SIM_QUANTITY aQuantity )
{
    for( SIM_PLOT_AXIS_Y* axis : m_axis_y )
    {
        if( axis->m_quantity == aQuantity )
        {
            return axis;
        }
    }
    return nullptr;
}
void SIM_PLOT_PANEL::addAxeY( SIM_QUANTITY aQuantity )
{
    // A plot pane only has a single axis per quantity;
    if( GetAxeY( aQuantity ) )
    {
        return;
    }

    int left = 0;
    int right = 0;

    for( SIM_PLOT_AXIS_Y* axis : m_axis_y )
    {
        if( axis->GetAlign() == mpALIGN_LEFT )
        {
            left++;
        }
        else if( axis->GetAlign() == mpALIGN_RIGHT )
        {
            right++;
        }
    }

    int align = left <= right ? mpALIGN_LEFT : mpALIGN_RIGHT;

    switch( aQuantity )
    {
    case SIM_QUANTITY::CURRENT:
        m_axis_y.push_back(
                new SIM_PLOT_AXIS_Y( SIM_QUANTITY::CURRENT, _( "Current" ), wxT( "A" ), align ) );
        break;
    case SIM_QUANTITY::GAIN:
        m_axis_y.push_back(
                new SIM_PLOT_AXIS_Y( SIM_QUANTITY::GAIN, _( "Gain" ), wxT( "dBV" ), align ) );
        break;
    case SIM_QUANTITY::PHASE:
        m_axis_y.push_back(
                new SIM_PLOT_AXIS_Y( SIM_QUANTITY::PHASE, _( "Phase" ), wxT( "\u00B0" ), align ) );
        break;
    case SIM_QUANTITY::NOISE:
        m_axis_y.push_back( new SIM_PLOT_AXIS_Y( SIM_QUANTITY::NOISE, _( "noise [(V or A)^2/Hz]" ),
                                                 wxT( "" ), align ) );
        break;
    case SIM_QUANTITY::VOLTAGE:
    default:
        m_axis_y.push_back(
                new SIM_PLOT_AXIS_Y( SIM_QUANTITY::VOLTAGE, _( "Voltage" ), wxT( "V" ), align ) );
        break;
    }

    SIM_PLOT_AXIS_Y* axis = m_axis_y.back();

    if( m_axis_y.size() > 1 )
    {
        axis->SetMasterScale( m_master_axis );
        axis->SetTicks( m_master_axis->GetTicks() );
    }
    else
    {
        m_master_axis = axis;
        axis->SetTicks( m_axis_x->GetTicks() );
    }

    m_plotWin->AddLayer( axis );
    m_plotWin->UpdateAll();
}

void SIM_PLOT_PANEL::updateAxes()
{
    if( m_axis_x )
        return;

    switch( GetType() )
    {
        case ST_AC:
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::FREQUENCY, _( "Frequency" ), wxT( "Hz" ),
                                            mpALIGN_BOTTOM );
            m_axis_x->m_log = true;
            break;

        case ST_DC:
            prepareDCAxes();
            break;

        case ST_NOISE:
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::FREQUENCY, _( "Frequency" ), wxT( "Hz" ),
                                            mpALIGN_BOTTOM );
            m_axis_x->m_log = true;
            break;

        case ST_TRANSIENT:
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::TIME, _( "Time" ), wxT( "s" ),
                                            mpALIGN_BOTTOM );
            break;

        default:
            // suppress warnings
            break;
    }

    if( m_axis_x )
    {
        m_axis_x->SetTicks( false );
        m_axis_x->SetNameAlign ( mpALIGN_BOTTOM );

        m_plotWin->AddLayer( m_axis_x );
    }
}

void SIM_PLOT_PANEL::prepareDCAxes()
{
    wxString sim_cmd = getSimCommand().Lower();
    wxString rem;

    if( sim_cmd.StartsWith( ".dc", &rem ) )
    {
        wxChar ch;

        rem.Trim( false );

        try
        {
            ch = rem.GetChar( 0 );
        }
        catch( ... )
        {;}

        switch( ch )
        {
        // Make sure that we have a reliable default (even if incorrectly labeled)
        default:
        case 'v':
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::VOLTAGE, ( "Voltage (swept)" ),
                                            wxT( "V" ), mpALIGN_BOTTOM );
            break;
        case 'i':
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::CURRENT, _( "Current (swept)" ),
                                            wxT( "A" ), mpALIGN_BOTTOM );
            break;
        case 'r':
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::RESISTANCE, ( "Resistance (swept)" ),
                                            wxT( "\u03A9" ), mpALIGN_BOTTOM );
            break;
        case 't':
            m_axis_x = new SIM_PLOT_AXIS_X( SIM_QUANTITY::TEMPERATURE, _( "Temperature (swept)" ),
                                            wxT( "\u00B0C" ), mpALIGN_BOTTOM );
            break;
        }

        m_axis_y.clear();
    }
}


void SIM_PLOT_PANEL::UpdatePlotColors()
{
    // Update bg and fg colors:
    m_plotWin->SetColourTheme( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::BACKGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::FOREGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ) );

    // Update color of all traces
    for( auto& t : m_traces )
        if( t.second->GetCursor() )
            t.second->GetCursor()->SetPen(
                    wxPen( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::CURSOR ) ) );

    m_plotWin->UpdateAll();
}


void SIM_PLOT_PANEL::UpdateTraceStyle( TRACE* trace )
{
    int        type = trace->GetType();
    wxPenStyle penStyle = ( ( ( type & SPT_AC_PHASE ) || ( type & SPT_CURRENT ) ) && m_dotted_cp )
                                  ? wxPENSTYLE_DOT
                                  : wxPENSTYLE_SOLID;
    trace->SetPen( wxPen( trace->GetTraceColour(), 2, penStyle ) );
}


bool SIM_PLOT_PANEL::addTrace( const wxString& aTitle, const wxString& aName, int aPoints,
                               const double* aX, const double* aY, SIM_PLOT_TYPE aType )
{
    TRACE* trace = nullptr;
    wxString name = aTitle;

    updateAxes();

    // Find previous entry, if there is one
    auto prev = m_traces.find( name );
    bool addedNewEntry = ( prev == m_traces.end() );

    std::cout << name << std::endl;

    SIM_QUANTITY quantity = SIM_QUANTITY::VOLTAGE;

    if( SPT_AC_MAG & aType )
    {
        quantity = SIM_QUANTITY::GAIN;
    }
    else if( SPT_AC_PHASE & aType )
    {
        quantity = SIM_QUANTITY::PHASE;
    }
    else if( SPT_VOLTAGE & aType )
    {
        quantity = SIM_QUANTITY::VOLTAGE;
    }
    else if( SPT_CURRENT & aType )
    {
        quantity = SIM_QUANTITY::CURRENT;
    }
    else
    {
        std::cerr << "Unhandled Y axis" << std::endl;
    }

    if( addedNewEntry )
    {
        // New entry
        addAxeY( quantity );

        trace = new TRACE( aName, aType, quantity );
        trace->SetTraceColour( m_colors.GenerateColor( m_traces ) );
        UpdateTraceStyle( trace );
        m_traces[name] = trace;

        // It is a trick to keep legend & coords always on the top
        for( mpLayer* l : m_topLevel )
            m_plotWin->DelLayer( l );

        m_plotWin->AddLayer( (mpLayer*) trace );

        for( mpLayer* l : m_topLevel )
            m_plotWin->AddLayer( l );
    }
    else
    {
        trace = prev->second;
    }

    std::vector<double> tmp( aY, aY + aPoints );

    if( GetType() == ST_AC )
    {
        if( aType & SPT_AC_PHASE )
        {
            for( int i = 0; i < aPoints; i++ )
                tmp[i] = tmp[i] * 180.0 / M_PI;                 // convert to degrees
        }
        else
        {
            for( int i = 0; i < aPoints; i++ )
            {
                // log( 0 ) is not valid.
                if( tmp[i] != 0 )
                    tmp[i] = 20 * log( tmp[i] ) / log( 10.0 );  // convert to dB
            }
        }
    }

    trace->SetData( std::vector<double>( aX, aX + aPoints ), tmp );

    trace->SetScale( m_axis_x, GetAxeY( quantity ) );

    m_plotWin->UpdateAll();

    return addedNewEntry;
}


bool SIM_PLOT_PANEL::deleteTrace( const wxString& aName )
{
    auto it = m_traces.find( aName );

    if( it != m_traces.end() )
    {
        TRACE* trace = it->second;
        m_traces.erase( it );

        if( CURSOR* cursor = trace->GetCursor() )
            m_plotWin->DelLayer( cursor, true );

        m_plotWin->DelLayer( trace, true, true );

        SIM_PLOT_AXIS_Y* axis = GetAxeY( trace->m_quantity );

        if( axis )
        {
            int cnt = 0;
            for( auto& t : m_traces )
            {
                if( t.second->m_quantity == trace->m_quantity )
                {
                    cnt++;
                }
            }

            if( cnt < 1 )
            {
                if( ( axis == m_axis_y.front() ) && ( m_axis_y.size() > 1 ) )
                {
                    m_master_axis = m_axis_y.at( 1 );
                    m_master_axis->SetTicks( !IsGridShown() );

                    for( SIM_PLOT_AXIS_Y* ax : m_axis_y )
                    {
                        ax->SetMasterScale( m_master_axis );
                    }
                }

                m_master_axis->SetMasterScale( nullptr );

                // The current system cannot draw a grid if the only axis is on the right...
                m_master_axis->SetAlign( mpALIGN_LEFT );

                m_plotWin->DelLayer( axis, false, true );

                m_axis_y.erase( std::find( m_axis_y.begin(), m_axis_y.end(), axis ) );

                m_plotWin->UpdateAll();
            }
        }

        ResetScales();

        return true;
    }

    return false;
}


void SIM_PLOT_PANEL::deleteAllTraces()
{
    for( auto& t : m_traces )
    {
        deleteTrace( t.first );
    }

    m_traces.clear();
}


bool SIM_PLOT_PANEL::HasCursorEnabled( const wxString& aName ) const
{
    TRACE* t = GetTrace( aName );

    return t ? t->HasCursor() : false;
}


void SIM_PLOT_PANEL::EnableCursor( const wxString& aName, bool aEnable )
{
    TRACE* t = GetTrace( aName );

    if( t == nullptr || t->HasCursor() == aEnable )
        return;

    if( aEnable )
    {
        CURSOR* c = new CURSOR( t, this );
        int     plotCenter = GetPlotWin()->GetMarginLeft()
                         + ( GetPlotWin()->GetXScreen() - GetPlotWin()->GetMarginLeft()
                                   - GetPlotWin()->GetMarginRight() )
                                   / 2;
        c->SetX( plotCenter );
        c->SetPen( wxPen( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::CURSOR ) ) );
        t->SetCursor( c );
        m_plotWin->AddLayer( c );
    }
    else
    {
        CURSOR* c = t->GetCursor();
        t->SetCursor( nullptr );
        m_plotWin->DelLayer( c, true );
    }

    // Notify the parent window about the changes
    wxQueueEvent( GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
}


void SIM_PLOT_PANEL::ResetScales()
{
    if( m_axis_x )
        m_axis_x->ResetDataRange();

    for (mpScaleY* axis: m_axis_y )
    {
        axis->ResetDataRange();
    }
   
    for( auto t : m_traces )
        t.second->UpdateScales();
}


wxDEFINE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );
