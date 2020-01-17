/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef STYLED_TEXT_CTRL_INDETERMINATE_H
#define STYLED_TEXT_CTRL_INDETERMINATE_H

#include <wx/window.h>
#include <wx/stc/stc.h>

/**
* @brief wxStyledTextCtrl wrapper to allow "indeterminate state" display
*/
class STYLED_TEXT_CTRL_INDETERMINATE : public wxStyledTextCtrl
{
public:
    STYLED_TEXT_CTRL_INDETERMINATE( wxWindow* aParent, wxWindowID aId,
            const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
            long aStyle = 0, const wxString& aName = wxSTCNameStr );

    virtual ~STYLED_TEXT_CTRL_INDETERMINATE()
    {
    }

    wxString GetValue() const override;
    void SetValue( const wxString& aValue ) override;

    void SetIndeterminateAllowed( bool aYes )
    {
        m_canBeIndeterminate = aYes;
    }

protected:
    bool m_canBeIndeterminate = true;

    void onTextFocusGet( wxFocusEvent& aEvent );
    void onTextFocusLost( wxFocusEvent& aEvent );

    bool checkSetIndeterminateState( const wxString& aValue );
};


#endif /* STYLED_TEXT_CTRL_INDETERMINATE_H */
