/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <panel_pcbnew_simul_DC_plane_capacitance.h>
#include <pcb_edit_frame.h>
#include <widgets/paged_dialog.h>

#ifdef KICAD_SPARSELIZARD // we are in a different compilation unit. Thus, this hack
#include "fem/common/fem_descriptor.h"
#endif

PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE::PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE(
        PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE( aParent->GetTreebook() )
{
    m_frame = aFrame;
    m_board = aFrame->GetBoard();

    for( auto it = m_board->BeginNets(); it != m_board->EndNets(); ++it )
    {
        m_net1ComboBox->Append( it->GetNetname() );
        m_net2ComboBox->Append( it->GetNetname() );
    }
}

void PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE::OnRun( wxCommandEvent& event )
{
#ifdef KICAD_SPARSELIZARD // we are in a different compilation unit. Thus, this hack
#endif
}