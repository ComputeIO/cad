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

#ifndef DIALOG_TEXT_ITEM_PROPERTIES_H_
#define DIALOG_TEXT_ITEM_PROPERTIES_H_

#include <dialog_text_item_properties_base.h>

#include <widgets/unit_binder.h>
#include <sch_text.h>
#include <sch_validators.h>
#include <bitmaps.h>

class SCH_EDIT_FRAME;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;

class DIALOG_TEXT_ITEM_PROPERTIES : public DIALOG_TEXT_ITEM_PROPERTIES_BASE
{
public:
    DIALOG_TEXT_ITEM_PROPERTIES( SCH_EDIT_FRAME* parent, SCH_TEXT* aTextItem );
    ~DIALOG_TEXT_ITEM_PROPERTIES();

    void SetTitle( const wxString& aTitle ) override;

private:
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    //void OnEnterKey( wxCommandEvent& aEvent ) override;
    void OnFormattingHelp( wxHyperlinkEvent& aEvent ) override;

    void setShapeBitmap();
    void OnShapeChoice( wxCommandEvent& aEvent ) override { setShapeBitmap(); }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    static BITMAPS label_icons[];

    SCH_EDIT_FRAME*       m_Parent;
    SCH_TEXT*             m_CurrentText;
    wxWindow*             m_activeTextCtrl;
    wxTextEntry*          m_activeTextEntry;
    UNIT_BINDER           m_textWidth;
    UNIT_BINDER           m_textHeight;
    UNIT_BINDER           m_positionX;
    UNIT_BINDER           m_positionY;
    SCH_NETNAME_VALIDATOR m_netNameValidator;
    SCINTILLA_TRICKS*     m_scintillaTricks;

    HTML_MESSAGE_BOX*     m_helpWindow;
};

#endif // DIALOG_TEXT_ITEM_PROPERTIES_H_
