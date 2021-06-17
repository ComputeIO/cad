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
    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, m_board );
    descriptor->m_reporter = new STDOUT_REPORTER();
    descriptor->m_dim = FEM_SIMULATION_DIMENSION::SIMUL3D;
    descriptor->m_simulationType = FEM_SIMULATION_TYPE::DC;
    descriptor->m_requiresDielectric = true;
    descriptor->m_simulateDielectric = true;
    descriptor->m_simulateConductor = false;

    wxString net1Name = m_net1ComboBox->GetStringSelection();
    wxString net2Name = m_net2ComboBox->GetStringSelection();

    NETINFO_ITEM* net1 = m_board->FindNet( net1Name );

    if( net1 == nullptr )
    {
        m_resultText->SetLabel( "Error loading net1." );
        return;
    }
    NETINFO_ITEM* net2 = m_board->FindNet( net2Name );

    if( net2 == nullptr )
    {
        m_resultText->SetLabel( "Error loading net2." );
        return;
    }

    if( net1->GetNetCode() == net2->GetNetCode() )
    {
        m_resultText->SetLabel( "net1 and net2 have the same net code." );
        return;
    }

    FEM_PORT*            port1 = new FEM_PORT( net1 );
    FEM_PORT_CONSTRAINT* constraint1 = new FEM_PORT_CONSTRAINT();

    constraint1->m_type = FEM_PORT_CONSTRAINT_TYPE::CHARGE;
    constraint1->m_value = 1e-9; // C
    port1->m_type = FEM_PORT_TYPE::SOURCE;
    port1->m_constraint = *constraint1;
    descriptor->AddPort( port1 );

    FEM_PORT*            port2 = new FEM_PORT( net2 );
    FEM_PORT_CONSTRAINT* constraint2 = new FEM_PORT_CONSTRAINT();

    constraint2->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;
    constraint2->m_value = 0; // V
    port2->m_type = FEM_PORT_TYPE::SOURCE;
    port2->m_constraint = *constraint2;
    descriptor->AddPort( port2 );

    FEM_RESULT_VALUE* r_capacitance =
            new FEM_RESULT_VALUE( FEM_VALUE_TYPE::CAPACITANCE, port1, port2 );

    if( !( r_capacitance )->IsInitialized() )
    {
        m_resultText->SetLabel( "Could not initialize capacitance result." );
        return;
    }

    if( !descriptor->AddResult( r_capacitance ) )
    {
        m_resultText->SetLabel( "Could not add capacitance result to descriptor." );
        return;
    }

    if( !descriptor->Run() )
    {
        m_resultText->SetLabel( "Simulation failed." );
        return;
    }

    if( !r_capacitance->m_valid )
    {
        m_resultText->SetLabel( "Result is invalid." );
        return;
    }

    m_resultText->SetLabel( "Simulated capacitance is "
                            + std::to_string( r_capacitance->m_value ) );

#endif
}