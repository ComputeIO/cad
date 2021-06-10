/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef CLASS_TEXT_LABEL_H
#define CLASS_TEXT_LABEL_H


#include <eda_text.h>
#include <sch_item.h>
#include <sch_field.h>
#include <sch_connection.h> // for CONNECTION_TYPE


class NETLIST_OBJECT_LIST;
class HTML_MESSAGE_BOX;


/*
 * Shape/Type of #SCH_HIERLABEL and #SCH_GLOBALLABEL.
 */
enum class PINSHEETLABEL_SHAPE
{
    PS_INPUT, // use "PS_INPUT" instead of "INPUT" to avoid colliding
              // with a Windows header on msys2
    PS_OUTPUT,
    PS_BIDI,
    PS_TRISTATE,
    PS_UNSPECIFIED
};


extern const char* SheetLabelType[]; /* names of types of labels */


class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_TEXT( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );

    /**
     * Clone \a aText into a new object.  All members are copied as is except
     * for the #m_isDangling member which is set to false.  This prevents newly
     * copied objects derived from #SCH_TEXT from having their connection state
     * improperly set.
     */
    SCH_TEXT( const SCH_TEXT& aText );

    ~SCH_TEXT() {}

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXT_T == aItem->Type();
    }

    virtual wxString GetClass() const override { return wxT( "SCH_TEXT" ); }

    /**
     * Return the set of contextual text variable tokens for this text item.
     *
     * @param[out] aVars
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    wxString GetShownText( int aDepth = 0 ) const override;

    /**
     * Increment the label text, if it ends with a number.
     *
     * @param aIncrement = the increment value to add to the number ending the text.
     */
    bool IncrementLabel( int aIncrement );


    /**
     * Set angle and associated horizontal alignment.
     *
     * Angle/alignment associations correspond to old "spin style" assignments
     * and are as follows:
     *
     * 0 degrees => TEXT_ATTRIBUTES::H_LEFT == LABEL_SPIN_STYLE::RIGHT (right?)
     * 90 degrees => TEXT_ATTRIBUTES::H_LEFT == LABEL_SPIN_STYLE::UP
     * 180 degrees => TEXT_ATTRIBUTES::H_RIGHT == LABEL_SPIN_STYLE::LEFT (sic)
     * 270 degrees => TEXT_ATTRIBUTES::H_RIGHT == LABEL_SPIN_STYLE::BOTTOM
     *
     * 0 degrees, H_LEFT aligned is normal horizontal text, aligned left.
     * 180 degrees, H_LEFT aligned is normal horizontal text,
     * aligned right - assuming KeepUpright is set for the text item.
     *
     * @param aAngle New angle for the item
     */
    void SetAlignedAngle( const EDA_ANGLE& aAngle )
    {
        SetTextAngle( aAngle );
        switch( aAngle.AsDegrees() )
        {
        default:
        case 0:
        case 90: Align( TEXT_ATTRIBUTES::H_LEFT ); break;
        case 180:
        case 270: Align( TEXT_ATTRIBUTES::H_RIGHT ); break;
        }
    }

    /**
     * Set angle/horizontal alignment from legacy "spin style".
     * Used in legacy reader.
     *
     * @param int corresponding to old spin style
     */
    void SetLegacySpinStyle( int aSpinStyle )
    {
        switch( aSpinStyle )
        {
            case 0: // LEFT
                SetAlignedAngle( EDA_ANGLE::ANGLE_180 );
                break;
            case 1: // UP
                SetAlignedAngle( EDA_ANGLE::ANGLE_90 );
                break;
            default:
            case 2: // RIGHT
                SetAlignedAngle( EDA_ANGLE::ANGLE_0 );
                break;
            case 3: // BOTTOM
                SetAlignedAngle( EDA_ANGLE::ANGLE_270 );
                break;
        }
    }

    /**
     * Get legacy "spin style" from angle.
     * Ignores horizontal alignment.
     * Used in legacy writer.
     * @return int corresponding to old spin style
     */
    int GetLegacySpinStyle() const
    {
        switch( GetTextEdaAngle().AsDegrees() )
        {
            default:
            case 0:
                return 2; // RIGHT
            case 90:
                return 1; // UP
            case 180:
                return 0; // LEFT
            case 270:
                return 3; // BOTTOM
        }
    }

    PINSHEETLABEL_SHAPE GetShape() const { return m_shape; }

    void SetShape( PINSHEETLABEL_SHAPE aShape ) { m_shape = aShape; }

    /**
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     *
     * @return the offset between the SCH_TEXT position and the text itself position
     */
    virtual wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset ) override;

    /**
     * Calculate the graphic shape (a polygon) associated to the text.
     *
     * @param aPoints A buffer to fill with polygon corners coordinates
     * @param Pos Position of the shape, for texts and labels: do nothing
     */
    virtual void CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                     std::vector<wxPoint>& aPoints, const wxPoint& Pos ) const
    {
        aPoints.clear();
    }

    void SwapData( SCH_ITEM* aItem ) override;

    const EDA_RECT GetBoundingBox() const override;

    bool operator<( const SCH_ITEM& aItem ) const override;

    int GetTextOffset( const RENDER_SETTINGS* aSettings = nullptr ) const;

    int GetPenWidth() const override;

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector ) override { EDA_TEXT::Offset( aMoveVector ); }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const wxPoint& aCenter ) override;

    virtual void Rotate90( bool aClockwise );

    virtual void MirrorAcrossXAxis()
    {
        SetTextAngle( GetTextEdaAngle().MirrorAcrossXAxis() );
        FlipHorizontalAlignment();
    }

    virtual void MirrorAcrossYAxis()
    {
        SetTextAngle( GetTextEdaAngle().MirrorAcrossYAxis() );
        FlipHorizontalAlignment();
    }

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const wxFindReplaceData& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    void GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH*           aPath = nullptr ) override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool aIsDangling ) { m_isDangling = aIsDangling; }

    std::vector<wxPoint> GetConnectionPoints() const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    wxPoint GetPosition() const override { return EDA_TEXT::GetTextPos(); }
    void    SetPosition( const wxPoint& aPosition ) override { EDA_TEXT::SetTextPos( aPosition ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) const override;

    EDA_ITEM* Clone() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static HTML_MESSAGE_BOX* ShowSyntaxHelp( wxWindow* aParentWindow );

    virtual int SchematicLayer() const { return LAYER_NOTES; }

    virtual bool ShowAlignment() const { return true; }

protected:
    COLOR4D doPrint( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, int aLayer );

    PINSHEETLABEL_SHAPE m_shape;

    /// True if not connected to another object if the object derive from SCH_TEXT
    /// supports connections.
    bool m_isDangling;

    CONNECTION_TYPE m_connectionType;
};


class SCH_LABEL_BASE : public SCH_TEXT
{
public:
    SCH_LABEL_BASE( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString,
                    KICAD_T aType = SCH_TEXT_T ) :
            SCH_TEXT( aPos, aText, aType )
    {
    }

    virtual bool ShowAlignment() const override { return false; }

protected:
};


class SCH_LABEL : public SCH_LABEL_BASE
{
public:
    SCH_LABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_LABEL() {}

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "SCH_LABEL" ); }

    bool IsType( const KICAD_T aScanTypes[] ) const override;

    const EDA_RECT GetBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T
               && ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    bool IsReplaceable() const override { return true; }

    EDA_ITEM* Clone() const override;

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    virtual int SchematicLayer() const override { return LAYER_LOCLABEL; }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};


class SCH_GLOBALLABEL : public SCH_LABEL_BASE
{
public:
    SCH_GLOBALLABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString );

    SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel );

    ~SCH_GLOBALLABEL() {}

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_GLOBAL_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "SCH_GLOBALLABEL" ); }

    EDA_ITEM* Clone() const override;

    void SwapData( SCH_ITEM* aItem ) override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;

    void Rotate( const wxPoint& aCenter ) override;
    void Rotate90( bool aClockwise ) override;

    void MirrorAcrossXAxis() override;
    void MirrorAcrossYAxis() override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

    wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    /**
     * Return the bounding box on the global label only, without taking in account
     * the intersheet references.
     */
    const EDA_RECT GetBoundingBoxBase() const;

    /**
     * Return the bounding box on the global label only, including the intersheet references.
     */
    const EDA_RECT GetBoundingBox() const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos ) const override;

    void UpdateIntersheetRefProps();
    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    bool ResolveTextVar( wxString* token, int aDepth ) const;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T
               && ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset ) override;

    void Plot( PLOTTER* aPlotter ) const override;

    SCH_FIELD* GetIntersheetRefs() { return &m_intersheetRefsField; }
    void       SetIntersheetRefs( const SCH_FIELD& aField ) { m_intersheetRefsField = aField; }

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    void Move( const wxPoint& aMoveVector ) override
    {
        SCH_TEXT::Move( aMoveVector );
        m_intersheetRefsField.Move( aMoveVector );
    }

    virtual int SchematicLayer() const override { return LAYER_GLOBLABEL; }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }

    SCH_FIELD m_intersheetRefsField;
};


class SCH_HIERLABEL : public SCH_LABEL_BASE
{
public:
    SCH_HIERLABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString,
                   KICAD_T aType = SCH_HIER_LABEL_T );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_HIERLABEL() {}

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_HIER_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "SCH_HIERLABEL" ); }

    wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos ) const override;
    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos, PINSHEETLABEL_SHAPE aShape ) const;

    const EDA_RECT GetBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T
               && ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    virtual int SchematicLayer() const override { return LAYER_HIERLABEL; }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};

#endif /* CLASS_TEXT_LABEL_H */
