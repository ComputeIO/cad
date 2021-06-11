/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <panel_pcbnew_simul_DC_power.h>
#include <panel_pcbnew_simul_DC_plane_capacitance.h>
#include <panel_setup_text_and_graphics.h>
#include <panel_setup_constraints.h>
#include <dialogs/panel_setup_netclasses.h>
#include <panel_setup_tracks_and_vias.h>
#include <panel_setup_mask_and_paste.h>
#include <../board_stackup_manager/panel_board_stackup.h>
#include <../board_stackup_manager/panel_board_finish.h>
#include <confirm.h>
#include <kiface_i.h>
#include <drc/drc_item.h>
#include <dialog_import_settings.h>
#include <io_mgr.h>
#include <dialogs/panel_setup_severities.h>
#include <panel_text_variables.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <widgets/infobar.h>
#include <widgets/resettable_panel.h>
#include <wildcards_and_files_ext.h>

#include "dialog_pcbnew_simul.h"
#include "panel_setup_rules.h"

using std::placeholders::_1;

DIALOG_PCBNEW_SIMUL::DIALOG_PCBNEW_SIMUL( PCB_EDIT_FRAME* aFrame ) :
        PAGED_DIALOG( aFrame, _( "Simulation Setup" ), false,
                      _( "Import Settings from Another Board..." ) ),
        m_frame( aFrame )
{
    BOARD* board = aFrame->GetBoard();

    m_DCpower = new PANEL_PCBNEW_SIMUL_DC_POWER( this, aFrame );
    m_DCPlaneCapacitance = new PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE( this, aFrame );


    m_treebook->AddPage( new wxPanel( this ), _( "Simulations" ) );

    /*
     * WARNING: Code currently relies on the layers setup coming before the physical stackup panel,
     * and thus transferring data to the board first.  See comment in
     * PANEL_SETUP_BOARD_STACKUP::TransferDataFromWindow and rework this logic if it is determined
     * that the order of these pages should be changed.
     */
    m_treebook->AddPage( m_DCpower, _( "DC Power Analysis" ) );
    m_treebook->AddPage( m_DCPlaneCapacitance, _( "Plane Capacitance" ) );

    // Connect Events
    m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_PCBNEW_SIMUL::OnPageChange ), NULL, this );

    finishDialogSettings();
}


DIALOG_PCBNEW_SIMUL::~DIALOG_PCBNEW_SIMUL()
{
    m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                            wxBookCtrlEventHandler( DIALOG_PCBNEW_SIMUL::OnPageChange ), NULL,
                            this );
}


void DIALOG_PCBNEW_SIMUL::OnPageChange( wxBookCtrlEvent& event )
{
    /*
    if( event.GetSelection() == m_DCpower )
    {
        m_infoBar->Dismiss();
    }
    */
}


// Run Import Settings... action
void DIALOG_PCBNEW_SIMUL::OnAuxiliaryAction( wxCommandEvent& event )
{
}
