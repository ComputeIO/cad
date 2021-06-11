/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_H
#define PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_H

#include <widgets/unit_binder.h>
#include <panel_pcbnew_simul_DC_plane_capacitance_base.h>
#include <board_design_settings.h>
#include <board.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew_id.h>
#include <widgets/paged_dialog.h>

class BOARD;
class BOARD_DESIGN_SETTINGS;
class PAGED_DIALOG;
class PCB_EDIT_FRAME;
class wxCommandEvent;

class PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE : public PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE
{
private:
    PCB_EDIT_FRAME* m_frame;
    BOARD*          m_board;

public:
public:
    PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame );
    ~PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE(){};
    //void onNetSelect( wxCommandEvent& event ) override;
    void OnRun( wxCommandEvent& event ) override;

private:
};

#endif //PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_H
