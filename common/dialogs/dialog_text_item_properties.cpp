/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_text_item_properties.h>
#include <wx/fontdlg.h>
#include <font/font.h>
#include <eda_text.h>

DIALOG_TEXT_ITEM_PROPERTIES::DIALOG_TEXT_ITEM_PROPERTIES( wxWindow* aParent ) :
        DIALOG_TEXT_ITEM_PROPERTIES_BASE( aParent ),
        m_edaText( nullptr )
{
}


DIALOG_TEXT_ITEM_PROPERTIES::DIALOG_TEXT_ITEM_PROPERTIES( wxWindow* aParent, EDA_TEXT* aText ) :
        DIALOG_TEXT_ITEM_PROPERTIES_BASE( aParent ),
        m_edaText( aText )
{
}


void DIALOG_TEXT_ITEM_PROPERTIES::OnShowFontDialog( wxCommandEvent& aEvent )
{
    wxFontData fontData;

    fontData.SetShowHelp( true );

    wxFontDialog* fontDialog = new wxFontDialog( this, fontData );
    if( fontDialog->ShowModal() == wxID_OK )
    {
        wxFont theFont = fontDialog->GetFontData().GetChosenFont();
        bool   bold = false;
        bool   italic = false;
        switch( theFont.GetStyle() )
        {
        case wxFONTSTYLE_ITALIC:
        case wxFONTSTYLE_SLANT: italic = true; break;
        default: break;
        }
        switch( theFont.GetWeight() )
        {
        case wxFONTWEIGHT_BOLD: bold = true; break;
        default: break;
        }
#ifdef OUTLINEFONT_DEBUG
        std::cerr << "DIALOG_PCB_TEXT_PROPERTIES::OnShowFontDialog() face name \""
                  << theFont.GetFaceName() << "\"" << ( bold ? "bold " : "" )
                  << ( italic ? "italic " : "" ) << std::endl;
#endif
        KIFONT::FONT* font = KIFONT::FONT::GetFont( theFont.GetFaceName(), bold, italic );
#ifdef OUTLINEFONT_DEBUG
        std::cerr << "DIALOG_PCB_TEXT_PROPERTIES::OnShowFontDialog() face \"" << theFont.GetFaceName()
                  << "\" font \"" << font->Name() << "\"" << std::endl;
#endif
        m_FontCtrl->SetValue( font->Name() );
        m_FontBold->SetValue( bold );
        m_FontItalic->SetValue( italic );
    }
}


void DIALOG_TEXT_ITEM_PROPERTIES::SetFontByName( const wxString& aFontName, bool aBold, bool aItalic )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "DIALOG_PCB_TEXT_PROPERTIES::SetFontByName( \"" << aFontName << "\", "
              << ( aBold ? "true, " : "false, " ) << ( aItalic ? "true" : "false" ) << " )"
              << std::endl;
#endif
    m_edaText->SetFont( KIFONT::FONT::GetFont( aFontName, aBold, aItalic ) );
    m_FontCtrl->SetValue( m_edaText->GetFont()->Name() );
    m_edaText->SetBold( aBold );
    m_edaText->SetItalic( aBold );
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "font is now \"" << m_edaText->GetFont()->Name() << "\"" << std::endl;
#endif
}
