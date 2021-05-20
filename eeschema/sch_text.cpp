/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_text.cpp
 * @brief Code for handling schematic texts (texts, labels, hlabels and global labels).
 */

#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <plotter.h>
#include <widgets/msgpanel.h>
#include <font/stroke_font.h>
#include <bitmaps.h>
#include <kicad_string.h>
#include <sch_text.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <default_values.h>
#include <wx/debug.h>
#include <dialogs/html_messagebox.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <core/mirror.h>
#include <dialog_helpers.h>
#include <trigo.h>

using KIGFX::SCH_RENDER_SETTINGS;


bool IncrementLabelMember( wxString& name, int aIncrement )
{
    int  ii, nn;
    long number = 0;

    ii = name.Len() - 1;
    nn = 0;

    // No number found, but simply repeating the same label is valid

    if( !wxIsdigit( name.GetChar( ii ) ) )
        return true;

    while( ii >= 0 && wxIsdigit( name.GetChar( ii ) ) )
    {
        ii--;
        nn++;
    }

    ii++; /* digits are starting at ii position */
    wxString litt_number = name.Right( nn );

    if( litt_number.ToLong( &number ) )
    {
        number += aIncrement;

        // Don't let result go below zero

        if( number > -1 )
        {
            name.Remove( ii );
            name << number;
            return true;
        }
    }
    return false;
}


/* Coding polygons for global symbol graphic shapes.
 *  the first parml is the number of corners
 *  others are the corners coordinates in reduced units
 *  the real coordinate is the reduced coordinate * text half size
 */
static int TemplateIN_HN[] = { 6, 0, 0, -1, -1, -2, -1, -2, 1, -1, 1, 0, 0 };
static int TemplateIN_HI[] = { 6, 0, 0, 1, 1, 2, 1, 2, -1, 1, -1, 0, 0 };
static int TemplateIN_UP[] = { 6, 0, 0, 1, -1, 1, -2, -1, -2, -1, -1, 0, 0 };
static int TemplateIN_BOTTOM[] = { 6, 0, 0, 1, 1, 1, 2, -1, 2, -1, 1, 0, 0 };

static int TemplateOUT_HN[] = { 6, -2, 0, -1, 1, 0, 1, 0, -1, -1, -1, -2, 0 };
static int TemplateOUT_HI[] = { 6, 2, 0, 1, -1, 0, -1, 0, 1, 1, 1, 2, 0 };
static int TemplateOUT_UP[] = { 6, 0, -2, 1, -1, 1, 0, -1, 0, -1, -1, 0, -2 };
static int TemplateOUT_BOTTOM[] = { 6, 0, 2, 1, 1, 1, 0, -1, 0, -1, 1, 0, 2 };

static int TemplateUNSPC_HN[] = { 5, 0, -1, -2, -1, -2, 1, 0, 1, 0, -1 };
static int TemplateUNSPC_HI[] = { 5, 0, -1, 2, -1, 2, 1, 0, 1, 0, -1 };
static int TemplateUNSPC_UP[] = { 5, 1, 0, 1, -2, -1, -2, -1, 0, 1, 0 };
static int TemplateUNSPC_BOTTOM[] = { 5, 1, 0, 1, 2, -1, 2, -1, 0, 1, 0 };

static int TemplateBIDI_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int TemplateBIDI_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int TemplateBIDI_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int TemplateBIDI_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int Template3STATE_HN[] = { 5, 0, 0, -1, -1, -2, 0, -1, 1, 0, 0 };
static int Template3STATE_HI[] = { 5, 0, 0, 1, -1, 2, 0, 1, 1, 0, 0 };
static int Template3STATE_UP[] = { 5, 0, 0, -1, -1, 0, -2, 1, -1, 0, 0 };
static int Template3STATE_BOTTOM[] = { 5, 0, 0, -1, 1, 0, 2, 1, 1, 0, 0 };

static int* TemplateShape[5][4] = {
    { TemplateIN_HN, TemplateIN_UP, TemplateIN_HI, TemplateIN_BOTTOM },
    { TemplateOUT_HN, TemplateOUT_UP, TemplateOUT_HI, TemplateOUT_BOTTOM },
    { TemplateBIDI_HN, TemplateBIDI_UP, TemplateBIDI_HI, TemplateBIDI_BOTTOM },
    { Template3STATE_HN, Template3STATE_UP, Template3STATE_HI, Template3STATE_BOTTOM },
    { TemplateUNSPC_HN, TemplateUNSPC_UP, TemplateUNSPC_HI, TemplateUNSPC_BOTTOM }
};


static int* getTemplateShape( PINSHEETLABEL_SHAPE aShape, EDA_ANGLE aAngle )
{
    int direction = 0;
    switch( aAngle.AsDegrees() )
    {
    case 0:
    default: direction = 0; break;
    case 90: direction = 1; break;
    case 180: direction = 2; break;
    case 270: direction = 3; break;
    }

    return TemplateShape[static_cast<int>( aShape )][direction];
}


SCH_TEXT::SCH_TEXT( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
        SCH_ITEM( NULL, aType ), EDA_TEXT( text ), m_shape( PINSHEETLABEL_SHAPE::PS_INPUT ),
        m_isDangling( false ), m_connectionType( CONNECTION_TYPE::NONE )
{
    m_layer = LAYER_NOTES;

    SetTextPos( pos );
    SetMultilineAllowed( true );
    SetKeepUpright( true );
}


SCH_TEXT::SCH_TEXT( const SCH_TEXT& aText ) :
        SCH_ITEM( aText ), EDA_TEXT( aText ), m_shape( aText.m_shape ),
        m_isDangling( aText.m_isDangling ), m_connectionType( aText.m_connectionType )
{
}


EDA_ITEM* SCH_TEXT::Clone() const
{
    return new SCH_TEXT( *this );
}


bool SCH_TEXT::IncrementLabel( int aIncrement )
{
    wxString text = GetText();
    bool     ReturnVal = IncrementLabelMember( text, aIncrement );

    if( ReturnVal )
        SetText( text );

    return ReturnVal;
}


wxPoint SCH_TEXT::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;

    // add an offset to x (or y) position to aid readability of text on a wire or line
    int dist = GetTextOffset( aSettings ) + GetPenWidth();

    switch( GetTextEdaAngle().AsDegrees() )
    {
    default:
    case 0: // Horizontal orientation
    case 180: text_offset.y = -dist; break;
    case 90: // Vertical orientation
    case 270: text_offset.x = -dist; break;
    }

#ifdef DEBUG
    std::cerr << "SCH_TEXT::GetSchematicTextOffset( ... ) " << *this << " offset " << text_offset
              << std::endl;
#endif
    return text_offset;
}


void SCH_TEXT::MirrorHorizontally( int aCenter )
{
    // Text is NOT really mirrored; it is moved to a suitable horizontal position
    MirrorAcrossYAxis();

    SetTextX( MIRRORVAL( GetTextPos().x, aCenter ) );
}


void SCH_TEXT::MirrorVertically( int aCenter )
{
    // Text is NOT really mirrored; it is moved to a suitable vertical position
    MirrorAcrossXAxis();

    SetTextY( MIRRORVAL( GetTextPos().y, aCenter ) );
}


void SCH_TEXT::Rotate( wxPoint aCenter )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aCenter, 900 );
    wxPoint offset = pt - GetTextPos();

    Rotate90( false );

    SetTextPos( GetTextPos() + offset );
}


void SCH_TEXT::Rotate90( bool aClockwise )
{
    if( aClockwise )
    {
        RotateCW();
    }
    else
    {
        RotateCCW();
    }
    SetDefaultAlignment();
}


void SCH_TEXT::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT* item = (SCH_TEXT*) aItem;

    std::swap( m_layer, item->m_layer );
    std::swap( m_shape, item->m_shape );
    std::swap( m_isDangling, item->m_isDangling );

    SwapText( *item );
    SwapEffects( *item );
}


bool SCH_TEXT::operator<( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto other = static_cast<const SCH_TEXT*>( &aItem );

    if( GetLayer() != other->GetLayer() )
        return GetLayer() < other->GetLayer();

    if( GetPosition().x != other->GetPosition().x )
        return GetPosition().x < other->GetPosition().x;

    if( GetPosition().y != other->GetPosition().y )
        return GetPosition().y < other->GetPosition().y;

    return GetText() < other->GetText();
}


int SCH_TEXT::GetTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    double ratio;

    if( aSettings )
        ratio = static_cast<const SCH_RENDER_SETTINGS*>( aSettings )->m_TextOffsetRatio;
    else if( Schematic() )
        ratio = Schematic()->Settings().m_TextOffsetRatio;
    else
        ratio = DEFAULT_TEXT_OFFSET_RATIO; // For previews (such as in Preferences), etc.

    int ret = KiROUND( ratio * GetTextSize().y );
#ifdef DEBUG
    std::cerr << "SCH_TEXT::GetTextOffset( ... ) " << *this << " offset " << ret << std::endl;
#endif
    return ret;
}


int SCH_TEXT::GetPenWidth() const
{
    return GetEffectiveTextPenWidth();
}


COLOR4D SCH_TEXT::doPrint( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, int aLayer )
{
    COLOR4D  color = aSettings->GetLayerColor( aLayer );
    wxPoint  offset( aOffset + GetSchematicTextOffset( aSettings ) );
    VECTOR2D position( GetTextPos().x + offset.x, GetTextPos().y + offset.y );
#ifdef DEBUG
    std::cerr << "SCH_TEXT::doPrint( ..., " << aOffset << ", " << aLayer << " ) \"" << GetText()
              << "\"@" << GetTextPos() << " color " << color << " offset " << offset << " position "
              << position << " angle " << GetTextEdaAngle() << " H alignment "
              << GetHorizontalAlignment() << std::endl;
#endif
    GRText( this, position, color );
    return color;
}


void SCH_TEXT::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    doPrint( aSettings, aOffset, m_layer );
}


void SCH_TEXT::GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return;

    DANGLING_END_ITEM item( LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


bool SCH_TEXT::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                    const SCH_SHEET_PATH*           aPath )
{
    // Normal text labels cannot be tested for dangling ends.
    if( Type() == SCH_TEXT_T )
        return false;

    bool previousState = m_isDangling;
    m_isDangling = true;
    m_connectionType = CONNECTION_TYPE::NONE;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
        case NO_CONNECT_END:
            if( GetTextPos() == item.GetPosition() )
            {
                m_isDangling = false;

                if( aPath && item.GetType() != PIN_END )
                    m_connected_items[*aPath].insert( static_cast<SCH_ITEM*>( item.GetItem() ) );
            }

            break;


        case BUS_START_END: m_connectionType = CONNECTION_TYPE::BUS; KI_FALLTHROUGH;

        case WIRE_START_END:
        {
            // These schematic items have created 2 DANGLING_END_ITEM one per end.  But being
            // a paranoid programmer, I'll check just in case.
            ii++;

            wxCHECK_MSG( ii < aItemList.size(), previousState != m_isDangling,
                         wxT( "Dangling end type list overflow.  Bad programmer!" ) );

            int accuracy = 1; // We have rounding issues with an accuracy of 0

            DANGLING_END_ITEM& nextItem = aItemList[ii];
            m_isDangling = !TestSegmentHit( GetTextPos(), item.GetPosition(),
                                            nextItem.GetPosition(), accuracy );

            if( !m_isDangling )
            {
                if( m_connectionType != CONNECTION_TYPE::BUS )
                    m_connectionType = CONNECTION_TYPE::NET;

                // Add the line to the connected items, since it won't be picked
                // up by a search of intersecting connection points
                if( aPath )
                {
                    auto sch_item = static_cast<SCH_ITEM*>( item.GetItem() );
                    AddConnectionTo( *aPath, sch_item );
                    sch_item->AddConnectionTo( *aPath, this );
                }
            }
        }
        break;

        default: break;
        }

        if( !m_isDangling )
            break;
    }

    if( m_isDangling )
        m_connectionType = CONNECTION_TYPE::NONE;

    return previousState != m_isDangling;
}


std::vector<wxPoint> SCH_TEXT::GetConnectionPoints() const
{
    // Normal text labels do not have connection points.  All others do.
    if( Type() == SCH_TEXT_T )
        return {};

    return { GetTextPos() };
}


const EDA_RECT SCH_TEXT::GetBoundingBox() const
{
    //EDA_RECT rect = GetTextBox();
    VECTOR2D bbox = GetFont()->BoundingBox( *this );
    wxSize   bboxSize( bbox.x, bbox.y );
    EDA_RECT rect( GetTextPos(), bboxSize );

    if( GetTextAngle() != 0 ) // Rotate rect
    {
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );
    }

    rect.Normalize();
    return rect;
}


wxString getElectricalTypeLabel( PINSHEETLABEL_SHAPE aType )
{
    switch( aType )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT: return _( "Input" );
    case PINSHEETLABEL_SHAPE::PS_OUTPUT: return _( "Output" );
    case PINSHEETLABEL_SHAPE::PS_BIDI: return _( "Bidirectional" );
    case PINSHEETLABEL_SHAPE::PS_TRISTATE: return _( "Tri-State" );
    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED: return _( "Passive" );
    default: return wxT( "???" );
    }
}


void SCH_TEXT::GetContextualTextVars( wxArrayString* aVars ) const
{
    if( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
        aVars->push_back( wxT( "CONNECTION_TYPE" ) );

    if( Type() == SCH_SHEET_PIN_T && m_parent )
        static_cast<SCH_SHEET*>( m_parent )->GetContextualTextVars( aVars );
}


wxString SCH_TEXT::GetShownText( int aDepth ) const
{
    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        if( ( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T
              || Type() == SCH_SHEET_PIN_T )
            && token->IsSameAs( wxT( "CONNECTION_TYPE" ) ) )
        {
            *token = getElectricalTypeLabel( GetShape() );
            return true;
        }

        if( Type() == SCH_SHEET_PIN_T && m_parent )
        {
            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( m_parent );

            if( sheet->ResolveTextVar( token, aDepth ) )
                return true;
        }

        if( Type() == SCH_TEXT_T )
        {
            if( token->Contains( ':' ) )
            {
                if( Schematic()->ResolveCrossReference( token, aDepth ) )
                    return true;
            }
            else
            {
                SCHEMATIC* schematic = Schematic();
                SCH_SHEET* sheet = schematic ? schematic->CurrentSheet().Last() : nullptr;

                if( sheet && sheet->ResolveTextVar( token, aDepth + 1 ) )
                    return true;
            }
        }

        return false;
    };

    bool     processTextVars = false;
    wxString text = EDA_TEXT::GetShownText( &processTextVars );

    if( processTextVars )
    {
        wxCHECK_MSG( Schematic(), wxEmptyString, "No parent SCHEMATIC set for SCH_TEXT!" );

        PROJECT* project = nullptr;

        if( Schematic() )
            project = &Schematic()->Prj();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &textResolver, nullptr, project );
    }

    return text;
}


wxString SCH_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Graphic Text '%s'" ), ShortenedShownText() );
}


BITMAPS SCH_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


bool SCH_TEXT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );
    return bBox.Contains( aPosition );
}


bool SCH_TEXT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( aAccuracy );

    if( aContained )
        return aRect.Contains( bBox );

    return aRect.Intersects( bBox );
}


void SCH_TEXT::Plot( PLOTTER* aPlotter ) const
{
    static std::vector<wxPoint> s_poly;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    SCH_CONNECTION*  connection = Connection();
    int              layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;
    COLOR4D          color = settings->GetLayerColor( layer );
    int              penWidth = GetEffectiveTextPenWidth( settings->GetDefaultPenWidth() );

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );
    aPlotter->SetCurrentLineWidth( penWidth );

    if( IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString        strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        positions.reserve( strings_list.Count() );

        GetLinePositions( positions, (int) strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxPoint textpos = positions[ii] + GetSchematicTextOffset( aPlotter->RenderSettings() );
            wxString& txt = strings_list.Item( ii );
            aPlotter->Text( textpos, color, txt, GetTextAngle(), GetTextSize(),
                            GetHorizontalAlignment(), GetVerticalAlignment(), penWidth, IsItalic(),
                            IsBold() );
        }
    }
    else
    {
        wxPoint textpos = GetTextPos() + GetSchematicTextOffset( aPlotter->RenderSettings() );

        aPlotter->Text( textpos, color, GetShownText(), GetTextAngle(), GetTextSize(),
                        GetHorizontalAlignment(), GetVerticalAlignment(), penWidth, IsItalic(),
                        IsBold() );
    }

    // Draw graphic symbol for global or hierarchical labels
    CreateGraphicShape( aPlotter->RenderSettings(), s_poly, GetTextPos() );

    if( s_poly.size() )
        aPlotter->PlotPoly( s_poly, FILL_TYPE::NO_FILL, penWidth );
}


void SCH_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( Type() )
    {
    case SCH_TEXT_T: msg = _( "Graphic Text" ); break;
    case SCH_LABEL_T: msg = _( "Label" ); break;
    case SCH_GLOBAL_LABEL_T: msg = _( "Global Label" ); break;
    case SCH_HIER_LABEL_T: msg = _( "Hierarchical Label" ); break;
    case SCH_SHEET_PIN_T: msg = _( "Hierarchical Sheet Pin" ); break;
    default: return;
    }

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.push_back( MSG_PANEL_ITEM( msg, UnescapeString( GetText() ) ) );

    msg = TEXT_ATTRIBUTES::ToString( GetHorizontalAlignment() );
    aList.push_back( MSG_PANEL_ITEM( _( "Justification" ), msg, BROWN ) );

    msg = GetTextEdaAngle().AsDegreesString();
    aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, BROWN ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int      style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
        style += 2;

    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), textStyle[style] ) );

    // Display electrical type if it is relevant
    if( Type() == SCH_GLOBAL_LABEL_T || Type() == SCH_HIER_LABEL_T || Type() == SCH_SHEET_PIN_T )
    {
        msg = getElectricalTypeLabel( GetShape() );
        aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg ) );
    }

    // Display text size (X or Y value, with are the same value in Eeschema)
    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetTextWidth() );
    aList.push_back( MSG_PANEL_ITEM( _( "Size" ), msg ) );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );

    if( frame )
    {
        if( SCH_CONNECTION* conn = Connection() )
        {
            conn->AppendInfoToMsgPanel( aList );

            NET_SETTINGS&   netSettings = Schematic()->Prj().GetProjectFile().NetSettings();
            const wxString& netname = conn->Name( true );

            if( netSettings.m_NetClassAssignments.count( netname ) )
            {
                const wxString& netclassName = netSettings.m_NetClassAssignments[netname];
                aList.push_back( MSG_PANEL_ITEM( _( "Assigned Netclass" ), netclassName ) );
            }
        }
    }
}

#if defined( DEBUG )

void SCH_TEXT::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << " layer=\"" << m_layer << '"'
                                 << " shape=\"" << static_cast<int>( m_shape ) << '"'
                                 << " dangling=\"" << m_isDangling << '"' << '>'
                                 << TO_UTF8( GetText() ) << "</" << s.Lower().mb_str() << ">\n";
}

#endif


SCH_LABEL::SCH_LABEL( const wxPoint& pos, const wxString& text ) :
        SCH_TEXT( pos, text, SCH_LABEL_T )
{
    m_layer = LAYER_LOCLABEL;
    m_shape = PINSHEETLABEL_SHAPE::PS_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_LABEL::Clone() const
{
    return new SCH_LABEL( *this );
}


bool SCH_LABEL::IsType( const KICAD_T aScanTypes[] ) const
{
    static KICAD_T wireTypes[] = { SCH_LINE_LOCATE_WIRE_T, SCH_PIN_T, EOT };
    static KICAD_T busTypes[] = { SCH_LINE_LOCATE_BUS_T, EOT };

    if( SCH_ITEM::IsType( aScanTypes ) )
        return true;

    wxCHECK_MSG( Schematic(), false, "No parent SCHEMATIC set for SCH_LABEL!" );

    SCH_SHEET_PATH current = Schematic()->CurrentSheet();

    for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
    {
        if( *p == SCH_LABEL_LOCATE_WIRE_T )
        {
            wxASSERT( m_connected_items.count( current ) );

            for( SCH_ITEM* connection : m_connected_items.at( current ) )
            {
                if( connection->IsType( wireTypes ) )
                    return true;
            }
        }
        else if( *p == SCH_LABEL_LOCATE_BUS_T )
        {
            wxASSERT( m_connected_items.count( current ) );

            for( SCH_ITEM* connection : m_connected_items.at( current ) )
            {
                if( connection->IsType( busTypes ) )
                    return true;
            }
        }
    }

    return false;
}


const EDA_RECT SCH_LABEL::GetBoundingBox() const
{
    VECTOR2D bbox = GetFont()->BoundingBox( *this );
    wxSize   bboxSize( bbox.x, bbox.y );
    EDA_RECT rect( GetTextPos(), bboxSize );

    rect.Offset( 0, -GetTextOffset() );

    if( GetTextAngle() != 0.0 )
    {
        // Rotate rect
        wxPoint pos = rect.GetOrigin();
        wxPoint end = rect.GetEnd();

        RotatePoint( &pos, GetTextPos(), GetTextAngle() );
        RotatePoint( &end, GetTextPos(), GetTextAngle() );

        rect.SetOrigin( pos );
        rect.SetEnd( end );

        rect.Normalize();
    }

    // Labels have a position point that is outside of the TextBox
    rect.Merge( GetPosition() );

    return rect;
}


wxString SCH_LABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Label '%s'" ), ShortenedShownText() );
}


BITMAPS SCH_LABEL::GetMenuImage() const
{
    return BITMAPS::add_line_label;
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const wxPoint& pos, const wxString& text ) :
        SCH_TEXT( pos, text, SCH_GLOBAL_LABEL_T ), m_intersheetRefsField( { 0, 0 }, 0, this )
{
    m_layer = LAYER_GLOBLABEL;
    m_shape = PINSHEETLABEL_SHAPE::PS_BIDI;
    m_isDangling = true;
    SetMultilineAllowed( false );

    Align( TEXT_ATTRIBUTES::V_CENTER );

    m_intersheetRefsField.SetText( wxT( "${INTERSHEET_REFS}" ) );
    m_intersheetRefsField.SetLayer( LAYER_GLOBLABEL );
    m_intersheetRefsField.Align( TEXT_ATTRIBUTES::V_CENTER );
    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


SCH_GLOBALLABEL::SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel ) :
        SCH_TEXT( aGlobalLabel ), m_intersheetRefsField( { 0, 0 }, 0, this )
{
    m_intersheetRefsField = aGlobalLabel.m_intersheetRefsField;

    // Re-parent the fields, which before this had aGlobalLabel as parent
    m_intersheetRefsField.SetParent( this );

    m_fieldsAutoplaced = aGlobalLabel.m_fieldsAutoplaced;
}


EDA_ITEM* SCH_GLOBALLABEL::Clone() const
{
    return new SCH_GLOBALLABEL( *this );
}


void SCH_GLOBALLABEL::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXT::SwapData( aItem );

    SCH_GLOBALLABEL* globalLabel = static_cast<SCH_GLOBALLABEL*>( aItem );

    // Swap field data wholesale...
    std::swap( m_intersheetRefsField, globalLabel->m_intersheetRefsField );

    // ...and then reset parent pointers.
    globalLabel->m_intersheetRefsField.SetParent( globalLabel );
    m_intersheetRefsField.SetParent( this );
}


SEARCH_RESULT SCH_GLOBALLABEL::Visit( INSPECTOR aInspector, void* testData,
                                      const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;

    for( const KICAD_T* p = aFilterTypes; ( stype = *p ) != EOT; ++p )
    {
        // If caller wants to inspect my type
        if( stype == SCH_LOCATE_ANY_T || stype == Type() )
        {
            if( SEARCH_RESULT::QUIT == aInspector( this, NULL ) )
                return SEARCH_RESULT::QUIT;
        }

        if( stype == SCH_LOCATE_ANY_T || stype == SCH_FIELD_T )
        {
            if( SEARCH_RESULT::QUIT == aInspector( GetIntersheetRefs(), this ) )
                return SEARCH_RESULT::QUIT;
        }
    }

    return SEARCH_RESULT::CONTINUE;
}


void SCH_GLOBALLABEL::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    aFunction( &m_intersheetRefsField );
}


wxPoint SCH_GLOBALLABEL::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;
    int     dist = GetTextOffset( aSettings );

    switch( m_shape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:
    case PINSHEETLABEL_SHAPE::PS_BIDI:
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:
        dist += GetTextHeight() * 3 / 4; // Use three-quarters-height as proxy for triangle size
        break;

    case PINSHEETLABEL_SHAPE::PS_OUTPUT:
    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED:
    default: break;
    }

    switch( GetTextEdaAngle().AsDegrees() )
    {
    default:
    case 0: text_offset.x += dist; break;
    case 90: text_offset.y -= dist; break;
    case 180: text_offset.x -= dist; break;
    case 270: text_offset.y += dist; break;
    }

#ifdef DEBUG
    std::cerr << "SCH_GLOBALLABEL::GetSchematicTextOffset( ... ) \"" << GetText() << "\" angle "
              << GetTextEdaAngle() << " H alignment " << GetHorizontalAlignment() << " dist "
              << dist << " (" << GetTextOffset( aSettings ) << ") text_offset " << text_offset
              << std::endl;
#endif
    return text_offset;
}


void SCH_GLOBALLABEL::Rotate( wxPoint aCenter )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aCenter, 900 );
    wxPoint offset = pt - GetTextPos();

    Rotate90( false );

    SetTextPos( GetTextPos() + offset );
    m_intersheetRefsField.SetTextPos( m_intersheetRefsField.GetTextPos() + offset );
}


void SCH_GLOBALLABEL::Rotate90( bool aClockwise )
{
#ifdef DEBUG
    std::cerr << "SCH_GLOBALLABEL::Rotate90( " << ( aClockwise ? "CW" : "CCW" ) << " )"
              << std::endl;
#endif
    SCH_TEXT::Rotate90( aClockwise );

    if( m_intersheetRefsField.GetTextAngle() == TEXT_ANGLE_VERT )
    {
        if ( m_intersheetRefsField.GetHorizontalAlignment() == TEXT_ATTRIBUTES::H_LEFT )
        {
            if( !aClockwise )
                m_intersheetRefsField.Align( TEXT_ATTRIBUTES::H_RIGHT );

            m_intersheetRefsField.SetTextAngle( TEXT_ANGLE_HORIZ );
        }
        else if( m_intersheetRefsField.GetHorizontalAlignment() == TEXT_ATTRIBUTES::H_RIGHT )
        {
            if( !aClockwise )
                m_intersheetRefsField.Align( TEXT_ATTRIBUTES::H_LEFT );

            m_intersheetRefsField.SetTextAngle( TEXT_ANGLE_HORIZ );
        }
    }
    else if( m_intersheetRefsField.GetTextAngle() == TEXT_ANGLE_HORIZ )
    {
        if ( m_intersheetRefsField.GetHorizontalAlignment() == TEXT_ATTRIBUTES::H_LEFT )
        {
            if( aClockwise )
                m_intersheetRefsField.Align( TEXT_ATTRIBUTES::H_RIGHT );

            m_intersheetRefsField.SetTextAngle( TEXT_ANGLE_VERT );
        }
        else if( m_intersheetRefsField.GetHorizontalAlignment() == TEXT_ATTRIBUTES::H_RIGHT )
        {
            if( aClockwise )
                m_intersheetRefsField.Align( TEXT_ATTRIBUTES::H_LEFT );

            m_intersheetRefsField.SetTextAngle( TEXT_ANGLE_VERT );
        }
    }

    wxPoint pos = m_intersheetRefsField.GetTextPos();
    RotatePoint( &pos, GetPosition(), aClockwise ? -900 : 900 );
    m_intersheetRefsField.SetTextPos( pos );
}


void SCH_GLOBALLABEL::MirrorAcrossXAxis()
{
    SCH_TEXT::MirrorAcrossXAxis();

    if( m_intersheetRefsField.GetTextAngle() == TEXT_ANGLE_HORIZ )
    {
        m_intersheetRefsField.FlipHorizontalAlignment();
    }

    wxPoint pos = m_intersheetRefsField.GetTextPos();
    wxPoint delta = GetPosition() - pos;

    pos.x = GetPosition().x + delta.x;

    m_intersheetRefsField.SetTextPos( pos );
}


void SCH_GLOBALLABEL::MirrorAcrossYAxis()
{
    SCH_TEXT::MirrorAcrossYAxis();

    if( m_intersheetRefsField.GetTextAngle() == TEXT_ANGLE_VERT )
    {
        m_intersheetRefsField.FlipHorizontalAlignment();
    }

    wxPoint pos = m_intersheetRefsField.GetTextPos();
    wxPoint delta = GetPosition() - pos;

    pos.y = GetPosition().y + delta.y;

    m_intersheetRefsField.SetTextPos( pos );
}


void SCH_GLOBALLABEL::MirrorHorizontally( int aCenter )
{
    wxPoint old_pos = GetPosition();
    SCH_TEXT::MirrorHorizontally( aCenter );

    m_intersheetRefsField.FlipHorizontalAlignment();

    wxPoint pos = m_intersheetRefsField.GetTextPos();
    wxPoint delta = old_pos - pos;
    pos.x = GetPosition().x + delta.x;

    m_intersheetRefsField.SetPosition( pos );
}


void SCH_GLOBALLABEL::MirrorVertically( int aCenter )
{
    wxPoint old_pos = GetPosition();
    SCH_TEXT::MirrorVertically( aCenter );
    wxPoint pos = m_intersheetRefsField.GetTextPos();
    wxPoint delta = old_pos - pos;
    pos.y = GetPosition().y + delta.y;

    m_intersheetRefsField.SetPosition( pos );
}


void SCH_GLOBALLABEL::UpdateIntersheetRefProps()
{
    m_intersheetRefsField.SetTextSize( GetTextSize() );
    m_intersheetRefsField.SetItalic( IsItalic() );
    m_intersheetRefsField.SetBold( IsBold() );
    m_intersheetRefsField.SetTextThickness( GetTextThickness() );

    if( m_fieldsAutoplaced == FIELDS_AUTOPLACED_AUTO )
        AutoplaceFields( nullptr, false );
}


void SCH_GLOBALLABEL::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    int margin = GetTextOffset();
    int labelLen = GetBoundingBoxBase().GetSizeMax();
    int penOffset = GetPenWidth() / 2;

    // Set both axes to penOffset; we're going to overwrite the text axis below
    wxPoint                               offset( -penOffset, -penOffset );
    EDA_ANGLE                             angle;
    TEXT_ATTRIBUTES::HORIZONTAL_ALIGNMENT alignment;
    int                                   offsetValue = labelLen + margin / 2;

    switch( GetTextEdaAngle().AsDegrees() )
    {
    default:
    case 0:
        angle = EDA_ANGLE::ANGLE_0;
        alignment = TEXT_ATTRIBUTES::H_LEFT;
        offset.x = offsetValue;
        break;

    case 90:
        angle = EDA_ANGLE::ANGLE_90;
        alignment = TEXT_ATTRIBUTES::H_LEFT;
        offset.y = -offsetValue;
        break;

    case 180:
        angle = EDA_ANGLE::ANGLE_180;
        alignment = TEXT_ATTRIBUTES::H_RIGHT;
        offset.x = -offsetValue;
        break;

    case 270:
        angle = EDA_ANGLE::ANGLE_270;
        alignment = TEXT_ATTRIBUTES::H_RIGHT;
        offset.y = offsetValue;
        break;
    }

    m_intersheetRefsField.SetTextAngle( angle );
    m_intersheetRefsField.SetHorizontalAlignment( alignment );
    m_intersheetRefsField.SetTextPos( GetPosition() + offset );

    m_fieldsAutoplaced = FIELDS_AUTOPLACED_AUTO;
}


bool SCH_GLOBALLABEL::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( token->IsSameAs( wxT( "INTERSHEET_REFS" ) ) && Schematic() )
    {
        SCHEMATIC_SETTINGS& settings = Schematic()->Settings();
        wxString            ref;
        auto                it = Schematic()->GetPageRefsMap().find( GetText() );

        if( it == Schematic()->GetPageRefsMap().end() )
        {
            ref = "?";
        }
        else
        {
            std::vector<wxString> pageListCopy;

            pageListCopy.insert( pageListCopy.end(), it->second.begin(), it->second.end() );
            std::sort( pageListCopy.begin(), pageListCopy.end() );

            if( !settings.m_IntersheetRefsListOwnPage )
            {
                wxString currentPage = Schematic()->CurrentSheet().GetPageNumber();
                pageListCopy.erase(
                        std::remove( pageListCopy.begin(), pageListCopy.end(), currentPage ),
                        pageListCopy.end() );
            }

            if( ( settings.m_IntersheetRefsFormatShort ) && ( pageListCopy.size() > 2 ) )
            {
                ref.Append( wxString::Format( wxT( "%s..%s" ), pageListCopy.front(),
                                              pageListCopy.back() ) );
            }
            else
            {
                for( const wxString& pageNo : pageListCopy )
                    ref.Append( wxString::Format( wxT( "%s," ), pageNo ) );

                if( !ref.IsEmpty() && ref.Last() == ',' )
                    ref.RemoveLast();
            }
        }

        *token = settings.m_IntersheetRefsPrefix + ref + settings.m_IntersheetRefsSuffix;
        return true;
    }

    return false;
}


void SCH_GLOBALLABEL::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    static std::vector<wxPoint> s_poly;

    SCH_CONNECTION* connection = Connection();
    int             layer = ( connection && connection->IsBus() ) ? LAYER_BUS : m_layer;

    doPrint( aSettings, aOffset, layer );

    CreateGraphicShape( aSettings, s_poly, GetTextPos() + aOffset );

    wxDC*   DC = aSettings->GetPrintDC();
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    COLOR4D color = aSettings->GetLayerColor( layer );

    GRPoly( nullptr, DC, s_poly.size(), &s_poly[0], false, penWidth, color, color );

    if( Schematic()->Settings().m_IntersheetRefsShow )
        m_intersheetRefsField.Print( aSettings, aOffset );
}


void SCH_GLOBALLABEL::Plot( PLOTTER* aPlotter ) const
{
    SCH_TEXT::Plot( aPlotter );

    bool show = Schematic()->Settings().m_IntersheetRefsShow;

    if( show )
        m_intersheetRefsField.Plot( aPlotter );
}


void SCH_GLOBALLABEL::CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings,
                                          std::vector<wxPoint>& aPoints, const wxPoint& Pos ) const
{
    VECTOR2D bbox = GetFont()->BoundingBox( *this );

    int margin = GetTextOffset( aRenderSettings );
    //int halfSize = ( GetTextHeight() / 2 ) + margin;
    int halfSize = ( bbox.y / 2 ) + margin;
    int linewidth = GetFont()->IsOutline() ? 0 : GetPenWidth();
    //int symb_len = LenSize( GetShownText(), linewidth ) + 2 * margin;
    int symb_len = bbox.x + 2 * margin;

    int x = symb_len + linewidth + 3;
    int y = halfSize + linewidth + 3;

    aPoints.clear();

    // Create outline shape : 6 points
    aPoints.emplace_back( wxPoint( 0, 0 ) );
    aPoints.emplace_back( wxPoint( 0, -y ) );  // Up
    aPoints.emplace_back( wxPoint( -x, -y ) ); // left
    aPoints.emplace_back( wxPoint( -x, 0 ) );  // Up left
    aPoints.emplace_back( wxPoint( -x, y ) );  // left down
    aPoints.emplace_back( wxPoint( 0, y ) );   // down

    int x_offset = 0;

    switch( m_shape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        break;

    case PINSHEETLABEL_SHAPE::PS_OUTPUT: aPoints[3].x -= halfSize; break;

    case PINSHEETLABEL_SHAPE::PS_BIDI:
    case PINSHEETLABEL_SHAPE::PS_TRISTATE:
        x_offset = -halfSize;
        aPoints[0].x += halfSize;
        aPoints[3].x -= halfSize;
        break;

    case PINSHEETLABEL_SHAPE::PS_UNSPECIFIED:
    default: break;
    }

    // flip angle by 180 degrees
    int angle = GetTextEdaAngle().RotateCW().RotateCW().AsTenthsOfADegree();

    // Rotate outlines and move corners in real position
    for( wxPoint& aPoint : aPoints )
    {
        aPoint.x += x_offset;

        if( angle )
            RotatePoint( &aPoint, angle );

        aPoint += Pos;
    }

    aPoints.push_back( aPoints[0] ); // closing
}


const EDA_RECT SCH_GLOBALLABEL::GetBoundingBoxBase() const
{
    // build the bounding box on the global label only, without taking in account
    // the intersheets references, just the bounding box of the graphic shape
    int x = GetTextPos().x;
    int y = GetTextPos().y;
    int penWidth = GetFont()->IsOutline() ? 0 : GetEffectiveTextPenWidth();
    int margin = GetTextOffset();

    VECTOR2D bbox = GetFont()->BoundingBox( *this );

    int height = ( ( bbox.y * 15 ) / 10 ) + penWidth + margin;
    int length = bbox.x + height // add height for triangular shapes
                 - margin;       // margin added to height not needed here
    int dx;
    int dy;

    switch( GetTextEdaAngle().AsDegrees() )
    {
    case 0:
        dx = length;
        dy = height;
        y -= height / 2;
        break;
    case 90:
        dx = height;
        dy = -length;
        x -= height / 2;
        break;
    default:
    case 180:
        dx = -length;
        dy = height;
        y -= height / 2;
        break;
    case 270:
        dx = height;
        dy = length;
        x -= height / 2;
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );

    box.Normalize();
    return box;
}


const EDA_RECT SCH_GLOBALLABEL::GetBoundingBox() const
{
    // build the bounding box on the global label only, including the intersheets references
    // full bounding box if they are shown

    EDA_RECT box( GetBoundingBoxBase() );

    // Note: Schematic() can be null in preference preview panel
    if( Schematic() && Schematic()->Settings().m_IntersheetRefsShow )
    {
        box.Merge( m_intersheetRefsField.GetBoundingBox() );
        box.Normalize();
    }

    return box;
}


wxString SCH_GLOBALLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Global Label '%s'" ), ShortenedShownText() );
}


BITMAPS SCH_GLOBALLABEL::GetMenuImage() const
{
    return BITMAPS::add_glabel;
}


SCH_HIERLABEL::SCH_HIERLABEL( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
        SCH_TEXT( pos, text, aType )
{
    m_layer = LAYER_HIERLABEL;
    m_shape = PINSHEETLABEL_SHAPE::PS_INPUT;
    m_isDangling = true;
    SetMultilineAllowed( false );
}


EDA_ITEM* SCH_HIERLABEL::Clone() const
{
    return new SCH_HIERLABEL( *this );
}


void SCH_HIERLABEL::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxCHECK_RET( Schematic(), "No parent SCHEMATIC set for SCH_LABEL!" );

    SCH_CONNECTION* conn = Connection();
    bool            isBus = conn && conn->IsBus();
    int             layer = ( isBus ? LAYER_BUS : m_layer );
    int             penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    COLOR4D color = doPrint( aSettings, aOffset, layer );

    static std::vector<wxPoint> Poly;
    CreateGraphicShape( aSettings, Poly,
                        GetTextPos() + aOffset + GetSchematicTextOffset( aSettings ) );
    GRPoly( nullptr, aSettings->GetPrintDC(), Poly.size(), &Poly[0], false, penWidth, color,
            color );
}


void SCH_HIERLABEL::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<wxPoint>& aPoints, const wxPoint& aPos ) const
{
    CreateGraphicShape( aSettings, aPoints, aPos, m_shape );
}


void SCH_HIERLABEL::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<wxPoint>& aPoints, const wxPoint& aPos,
                                        PINSHEETLABEL_SHAPE aShape ) const
{
    int* Template = getTemplateShape( aShape, GetTextEdaAngle() );
    int  halfSize = GetTextHeight() / 2;
    int  imax = *Template;
    Template++;

    aPoints.clear();

    for( int ii = 0; ii < imax; ii++ )
    {
        wxPoint corner;
        corner.x = ( halfSize * ( *Template ) ) + aPos.x;
        Template++;

        corner.y = ( halfSize * ( *Template ) ) + aPos.y;
        Template++;

        aPoints.push_back( corner );
    }
}


const EDA_RECT SCH_HIERLABEL::GetBoundingBox() const
{
    int penWidth = GetEffectiveTextPenWidth();
    int margin = GetTextOffset();

    int x = GetTextPos().x;
    int y = GetTextPos().y;

    int height = GetTextHeight() + penWidth + margin;
    int length = LenSize( GetShownText(), penWidth ) + height; // add height for triangular shapes

    int dx, dy;

    switch( GetTextEdaAngle().AsDegrees() )
    {
    case 0:
        dx = length;
        dy = height;
        x -= Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;
    case 90:
        dx = height;
        dy = -length;
        x -= height / 2;
        y += Mils2iu( DANGLING_SYMBOL_SIZE );
        break;
    default:
    case 180:
        dx = -length;
        dy = height;
        x += Mils2iu( DANGLING_SYMBOL_SIZE );
        y -= height / 2;
        break;
    case 270:
        dx = height;
        dy = length;
        x -= height / 2;
        y -= Mils2iu( DANGLING_SYMBOL_SIZE );
        break;
    }

    EDA_RECT box( wxPoint( x, y ), wxSize( dx, dy ) );
    box.Normalize();
    return box;
}


wxPoint SCH_HIERLABEL::GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const
{
    wxPoint text_offset;
    int     dist = GetTextOffset( aSettings );

    dist += GetTextWidth();

    switch( GetTextEdaAngle().AsDegrees() )
    {
    default:
    case 0: text_offset.x = dist; break;
    case 180: text_offset.x = -dist; break;
    case 90: text_offset.y = -dist; break;
    case 270: text_offset.y = dist; break;
    }

    return text_offset;
}


wxString SCH_HIERLABEL::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Hierarchical Label '%s'" ), ShortenedShownText() );
}


BITMAPS SCH_HIERLABEL::GetMenuImage() const
{
    return BITMAPS::add_hierarchical_label;
}


HTML_MESSAGE_BOX* SCH_TEXT::ShowSyntaxHelp( wxWindow* aParentWindow )
{
    wxString msg =
#include "sch_text_help_md.h"
            ;

    HTML_MESSAGE_BOX* dlg = new HTML_MESSAGE_BOX( nullptr, _( "Syntax Help" ) );
    wxSize            sz( 320, 320 );

    dlg->SetMinSize( dlg->ConvertDialogToPixels( sz ) );
    dlg->SetDialogSizeInDU( sz.x, sz.y );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    dlg->m_htmlWindow->AppendToPage( html_txt );
    dlg->ShowModeless();

    return dlg;
}
