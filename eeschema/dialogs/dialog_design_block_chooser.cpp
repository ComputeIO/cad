/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <design_block.h>
#include <dialog_design_block_chooser.h>
#include <widgets/panel_design_block_chooser.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <sch_base_frame.h>
#include <core/kicad_algo.h>
#include <template_fieldnames.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>

std::mutex DIALOG_DESIGN_BLOCK_CHOOSER::g_Mutex;

DIALOG_DESIGN_BLOCK_CHOOSER::DIALOG_DESIGN_BLOCK_CHOOSER( SCH_BASE_FRAME*      aParent,
                                                          const LIB_ID*        aPreselect,
                                                          std::vector<LIB_ID>& aHistoryList ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Choose Design Block" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_chooserPanel = new PANEL_DESIGN_BLOCK_CHOOSER(
            aParent, this, aHistoryList,
            [this]()
            {
                EndModal( wxID_OK );
            },
            [this]()
            {
                EndModal( wxID_CANCEL );
            } );

    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    SetTitle( GetTitle()
              + wxString::Format( _( " (%d items loaded)" ), m_chooserPanel->GetItemCount() ) );

    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    m_repeatedPlacement = new wxCheckBox( this, wxID_ANY, _( "Place repeated copies" ) );
    m_repeatedPlacement->SetToolTip( _( "Place copies of the design block on subsequent clicks." ) );

    m_placeAsSheet = new wxCheckBox( this, wxID_ANY, _( "Place as sheet" ) );
    m_placeAsSheet->SetToolTip( _( "Place the design block as a new sheet." ) );

    buttonsSizer->Add( m_repeatedPlacement, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5 );
    buttonsSizer->Add( m_placeAsSheet, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 30 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK );
    wxButton*               cancelButton = new wxButton( this, wxID_CANCEL );

    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    buttonsSizer->Add( sdbSizer, 1, wxALL, 5 );

    sizer->Add( buttonsSizer, 0, wxEXPAND | wxLEFT, 5 );
    SetSizer( sizer );

    SetInitialFocus( m_chooserPanel->GetFocusTarget() );
    SetupStandardButtons();

    m_chooserPanel->FinishSetup();
    Layout();

    Bind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, m_chooserPanel );
}


DIALOG_DESIGN_BLOCK_CHOOSER::~DIALOG_DESIGN_BLOCK_CHOOSER()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_DesignBlockChooserPanel.repeated_placement = m_repeatedPlacement->GetValue();
        cfg->m_DesignBlockChooserPanel.place_as_sheet = m_placeAsSheet->GetValue();
    }
}


LIB_ID DIALOG_DESIGN_BLOCK_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_chooserPanel->GetSelectedLibId( aUnit );
}
