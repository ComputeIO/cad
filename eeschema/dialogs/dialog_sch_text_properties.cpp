/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <sch_edit_frame.h>
#include <base_units.h>
#include <sch_validators.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <gr_text.h>
#include <confirm.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <dialogs/html_messagebox.h>
#include <kicad_string.h>
#include <tool/actions.h>
#include <scintilla_tricks.h>
#include <bitmaps.h>
#include <string>
#include <dialog_sch_text_properties.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;

BITMAPS DIALOG_SCH_TEXT_PROPERTIES::label_icons[] = { BITMAPS::label_input, BITMAPS::label_output,
                                                      BITMAPS::label_bidirectional,
                                                      BITMAPS::label_tristate,
                                                      BITMAPS::label_passive };

//m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),

DIALOG_SCH_TEXT_PROPERTIES::DIALOG_SCH_TEXT_PROPERTIES( SCH_EDIT_FRAME* aParent,
                                                        SCH_TEXT*       aTextItem ) :
        DIALOG_TEXT_ITEM_PROPERTIES( aParent, aTextItem ),
        m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits, false ),
        m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits, false ),
        m_positionX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits, false ),
        m_positionY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits, false ),
        m_netNameValidator( true ), m_scintillaTricks( nullptr ), m_helpWindow( nullptr )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;

    m_statusLine->Show( false );
    m_cbLocked->Show( false );
    m_LayerSelectionCtrl->Show( false );

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T: SetTitle( _( "Global Label Properties" ) ); break;
    case SCH_HIER_LABEL_T: SetTitle( _( "Hierarchical Label Properties" ) ); break;
    case SCH_LABEL_T: SetTitle( _( "Label Properties" ) ); break;
    case SCH_SHEET_PIN_T: SetTitle( _( "Hierarchical Sheet Pin Properties" ) ); break;
    default: SetTitle( _( "Text Properties" ) ); break;
    }

    m_MultiLineText->SetEOLMode( wxSTC_EOL_LF );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_MultiLineText, wxT( "()" ) );

    if( m_CurrentText->IsMultilineAllowed() )
    {
        m_activeTextCtrl = m_MultiLineText;
        m_activeTextEntry = nullptr;

        m_SingleLineLabel->Show( false );
        m_SingleLineText->Show( false );
        m_NetLabel->Show( false );
        m_NetValue->Show( false );

        //m_textEntrySizer->AddGrowableRow( 0 );
    }
    else if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T || m_CurrentText->Type() == SCH_LABEL_T )
    {
        m_activeTextCtrl = m_NetValue;
        m_activeTextEntry = m_NetValue;

        m_SingleLineLabel->Show( false );
        m_SingleLineText->Show( false );
        m_MultiLineLabel->Show( false );
        m_MultiLineText->Show( false );

        m_NetValue->SetValidator( m_netNameValidator );
    }
    else
    {
        m_activeTextCtrl = m_SingleLineText;
        m_activeTextEntry = m_SingleLineText;

        m_MultiLineLabel->Show( false );
        m_MultiLineText->Show( false );
        m_NetLabel->Show( false );
        m_NetValue->Show( false );

        if( m_CurrentText->Type() != SCH_TEXT_T )
            m_SingleLineText->SetValidator( m_netNameValidator );

        m_NetValue->SetValidator( m_netNameValidator );
    }

    SetInitialFocus( m_activeTextCtrl );

    bool selectShape = m_CurrentText->Type() == SCH_GLOBAL_LABEL_T
                       || m_CurrentText->Type() == SCH_HIER_LABEL_T;
    m_ShapeLabel->Show( selectShape );
    m_Shape->Show( selectShape );
    m_ShapeBitmap->Show( selectShape );

    bool showNote = m_CurrentText->Type() == SCH_GLOBAL_LABEL_T;
    if( showNote )
    {
        wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
        infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
        m_note1->SetFont( infoFont );
        m_note2->SetFont( infoFont );
    }

    m_note1->Show( showNote );
    m_note2->Show( showNote );

    m_sdbSizerOK->SetDefault();
    Layout();

    m_MultiLineText->Bind( wxEVT_STC_CHARADDED, &DIALOG_SCH_TEXT_PROPERTIES::onScintillaCharAdded,
                           this );

    int    size = wxNORMAL_FONT->GetPointSize();
    wxFont fixedFont( size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

    for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
        m_MultiLineText->StyleSetFont( i, fixedFont );

    // Addresses a bug in wx3.0 where styles are not correctly set
    m_MultiLineText->StyleClearAll();

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // various versions have different controls so we want to store sizes for each version.
    m_hash_key = TO_UTF8( GetTitle() );


    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_SCH_TEXT_PROPERTIES::~DIALOG_SCH_TEXT_PROPERTIES()
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


void DIALOG_SCH_TEXT_PROPERTIES::SetTitle( const wxString& aTitle )
{
    // This class is shared for numerous tasks: a couple of single line labels and
    // multi-line text fields.  Since the desired size of the multi-line text field editor
    // is often larger, we retain separate sizes based on the dialog titles.
    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
        // labels can share retained settings probably.
        break;

    default: m_hash_key = TO_UTF8( aTitle ); m_hash_key += typeid( *this ).name();
    }

    DIALOG_TEXT_ITEM_PROPERTIES_BASE::SetTitle( aTitle );
}


bool DIALOG_SCH_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        SCHEMATIC& schematic = m_Parent->Schematic();

        // show text variable cross-references in a human-readable format
        m_MultiLineText->SetValue( schematic.ConvertKIIDsToRefs( m_CurrentText->GetText() ) );
    }
    else
    {
        // show control characters in a human-readable format
        m_activeTextEntry->SetValue( UnescapeString( m_CurrentText->GetText() ) );
    }

    if( m_NetValue->IsShown() )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString> existingLabels;
        SCH_SCREENS        allScreens( m_Parent->Schematic().Root() );

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( m_CurrentText->Type() ) )
            {
                auto textItem = static_cast<const SCH_TEXT*>( item );
                existingLabels.insert( UnescapeString( textItem->GetText() ) );
            }
        }

        wxArrayString existingLabelArray;

        for( const wxString& label : existingLabels )
            existingLabelArray.push_back( label );

        // existingLabelArray.Sort();
        m_NetValue->Append( existingLabelArray );
    }

    EDA_ANGLE textAngle( m_CurrentText->GetTextAngle(), EDA_ANGLE::TENTHS_OF_A_DEGREE );
#ifdef DEBUG
    std::cerr << "text angle is " << textAngle;
#endif
    textAngle.Normalize();
#ifdef DEBUG
    std::cerr << ", normalized " << textAngle << std::endl;
#endif

    // Set text options
    switch( textAngle.AsDegrees() )
    {
    case 0:
    default: m_OrientCtrl->SetSelection( 0 ); break;
    case 90: m_OrientCtrl->SetSelection( 1 ); break;
    case 180: m_OrientCtrl->SetSelection( 3 ); break;
    case 270: m_OrientCtrl->SetSelection( 2 ); break;
    }

    bool showJustify = m_CurrentText->IsMultilineAllowed();
    if( showJustify )
    {
        switch( m_CurrentText->GetHorizontalAlignment() )
        {
        case TEXT_ATTRIBUTES::H_LEFT: m_Justify->SetSelection( 0 ); break;
        case TEXT_ATTRIBUTES::H_CENTER: m_Justify->SetSelection( 1 ); break;
        case TEXT_ATTRIBUTES::H_RIGHT: m_Justify->SetSelection( 2 ); break;
        }
    }
    else
    {
        m_JustifyLabel->Show( false );
        m_Justify->Show( false );
    }

    m_Shape->SetSelection( static_cast<int>( m_CurrentText->GetShape() ) );
    setShapeBitmap();

    m_FontCtrl->SetValue( m_CurrentText->GetFont()->Name() );
    m_FontBold->SetValue( m_CurrentText->IsBold() );
    m_FontItalic->SetValue( m_CurrentText->IsItalic() );

    m_textWidth.SetValue( m_CurrentText->GetTextWidth() );
    m_textHeight.SetValue( m_CurrentText->GetTextHeight() );

    m_positionX.SetValue( m_CurrentText->GetPosition().x );
    m_positionY.SetValue( m_CurrentText->GetPosition().y );

    m_Mirrored->SetValue( m_CurrentText->IsMirrored() );

    m_FontLineSpacing->SetValue( wxString::Format( "%.1f", m_CurrentText->GetLineSpacing() ) );

    m_KeepUpright->SetValue( m_CurrentText->IsKeepUpright() );

    return true;
}


void DIALOG_SCH_TEXT_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent& aEvent )
{
    wxStyledTextCtrl* te = m_MultiLineText;
    wxArrayString     autocompleteTokens;
    int               text_pos = te->GetCurrentPos();
    int               start = te->WordStartPosition( text_pos, true );
    wxString          partial;

    auto textVarRef = [&]( int pos )
    {
        return pos >= 2 && te->GetCharAt( pos - 2 ) == '$' && te->GetCharAt( pos - 1 ) == '{';
    };

    // Check for cross-reference
    if( start > 1 && te->GetCharAt( start - 1 ) == ':' )
    {
        int refStart = te->WordStartPosition( start - 1, true );

        if( textVarRef( refStart ) )
        {
            partial = te->GetRange( start + 1, text_pos );

            wxString           ref = te->GetRange( refStart, start - 1 );
            SCH_SHEET_LIST     sheets = m_Parent->Schematic().GetSheets();
            SCH_REFERENCE_LIST refs;
            SCH_COMPONENT*     refSymbol = nullptr;

            sheets.GetSymbols( refs );

            for( size_t jj = 0; jj < refs.GetCount(); jj++ )
            {
                if( refs[jj].GetSymbol()->GetRef( &refs[jj].GetSheetPath(), true ) == ref )
                {
                    refSymbol = refs[jj].GetSymbol();
                    break;
                }
            }

            if( refSymbol )
                refSymbol->GetContextualTextVars( &autocompleteTokens );
        }
    }
    else if( textVarRef( start ) )
    {
        partial = te->GetTextRange( start, text_pos );

        m_CurrentText->GetContextualTextVars( &autocompleteTokens );

        SCHEMATIC* schematic = m_CurrentText->Schematic();

        if( schematic && schematic->CurrentSheet().Last() )
            schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_MultiLineText->SetFocus();
}


bool DIALOG_SCH_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textWidth.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    if( !m_textHeight.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    wxString text;

    /* save old text in undo list if not already in edit */
    if( m_CurrentText->GetEditFlags() == 0 )
        m_Parent->SaveCopyInUndoList( m_Parent->GetScreen(), m_CurrentText, UNDO_REDO::CHANGED,
                                      false );

    m_Parent->GetCanvas()->Refresh();

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        // convert any text variable cross-references to their UUIDs
        text = m_Parent->Schematic().ConvertRefsToKIIDs( m_MultiLineText->GetValue() );
    }
    else
    {
        // labels need escaping
        text = EscapeString( m_activeTextEntry->GetValue(), CTX_NETNAME );
    }

    if( !text.IsEmpty() )
    {
        m_CurrentText->SetText( text );
    }
    else if( !m_CurrentText->IsNew() )
    {
        DisplayError( this, _( "Label requires non-empty text." ) );
        return false;
    }

    switch( m_Justify->GetSelection() )
    {
    case 0:
    default: m_CurrentText->Align( TEXT_ATTRIBUTES::H_LEFT ); break;
    case 1: m_CurrentText->Align( TEXT_ATTRIBUTES::H_CENTER ); break;
    case 2: m_CurrentText->Align( TEXT_ATTRIBUTES::H_RIGHT ); break;
    }

    switch( m_OrientCtrl->GetSelection() )
    {
    case 0:
    default: m_CurrentText->SetTextAngle( 0 ); break;
    case 1: m_CurrentText->SetTextAngle( 900 ); break;
    case 2: m_CurrentText->SetTextAngle( 2700 ); break; // -90 degrees
    case 3: m_CurrentText->SetTextAngle( 1800 ); break;
    }

    m_CurrentText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );

    if( m_Shape )
        m_CurrentText->SetShape( (PINSHEETLABEL_SHAPE) m_Shape->GetSelection() );

    bool bold = m_FontBold->GetValue();
    bool italic = m_FontItalic->GetValue();
    m_CurrentText->SetFont( KIFONT::FONT::GetFont( m_FontCtrl->GetValue(), bold, italic ) );
    m_FontCtrl->SetValue( m_CurrentText->GetFont()->Name() );
    m_CurrentText->SetBold( bold );
    m_CurrentText->SetItalic( italic );

    int penWidth = bold ? GetPenSizeForBold( m_CurrentText->GetTextWidth() ) : 0;
    m_CurrentText->SetTextThickness( penWidth );

    m_CurrentText->SetLineSpacing( std::stod( m_FontLineSpacing->GetValue().ToStdString() ) );
    m_CurrentText->SetPosition( wxPoint( m_positionX.GetValue(), m_positionY.GetValue() ) );

    m_Parent->UpdateItem( m_CurrentText );
    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( m_CurrentText );
        label->UpdateIntersheetRefProps();
    }

    m_CurrentText->SetKeepUpright( m_KeepUpright->GetValue() );

    return true;
}


void DIALOG_SCH_TEXT_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}


void DIALOG_SCH_TEXT_PROPERTIES::setShapeBitmap()
{
    int selectedItem = m_Shape->GetSelection();

    if( selectedItem == wxNOT_FOUND )
    {
        // clear shape display
        m_ShapeBitmap->SetBitmap( wxBitmap() );
    }
    else
    {
        m_ShapeBitmap->SetBitmap( KiBitmap( label_icons[selectedItem] ) );
    }
}
