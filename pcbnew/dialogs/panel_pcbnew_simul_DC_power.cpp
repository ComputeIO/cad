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

#include <panel_pcbnew_simul_DC_power.h>
#include <pcb_edit_frame.h>
#include <widgets/paged_dialog.h>
#include "fem/common/fem_descriptor.h"

PANEL_PCBNEW_SIMUL_DC_POWER::PANEL_PCBNEW_SIMUL_DC_POWER( PAGED_DIALOG*   aParent,
                                                          PCB_EDIT_FRAME* aFrame ) :
        PANEL_PCBNEW_SIMUL_DC_POWER_BASE( aParent->GetTreebook() )
{
    m_frame = aFrame;
    m_board = aFrame->GetBoard();

  for (auto it = m_board->BeginNets(); it!=m_board->EndNets(); ++it)
  {
      m_netComboBox->Append( it->GetNetname() );
  }

  m_padGrid->SetColLabelValue( 0, "Component" );
  m_padGrid->SetColLabelValue( 1, "Pad" );
  m_padGrid->SetColLabelValue( 2, "Type" );
  m_padGrid->SetColLabelValue( 3, "Value" );
  m_padGrid->SetColLabelValue( 4, "Unit" );
}

void PANEL_PCBNEW_SIMUL_DC_POWER::onNetSelect( wxCommandEvent& event )
{
    wxString      netName = m_netComboBox->GetStringSelection();
    NETINFO_ITEM* net = m_board->FindNet( netName );

    if( net == nullptr )
        return;

    int netCode = net->GetNetCode();
    int nbRow = 0;

    m_padGrid->DeleteRows( 0, m_padGrid->GetNumberRows() );

    for( PAD* pad : m_board->GetPads() )
    {
        if( pad->GetNetCode() == netCode )
        {
            m_padGrid->AppendRows( 1 );
            m_padGrid->SetCellValue( nbRow, 0, pad->GetParent()->GetReference() );
            m_padGrid->SetCellValue( nbRow, 1, pad->GetName() );
            m_padGrid->SetCellEditor( nbRow, 2,
                                      new wxGridCellChoiceEditor( 3, m_portTypes, false ) );
            m_padGrid->SetCellValue( nbRow, 2, "passive" );
            m_padGrid->SetCellEditor( nbRow, 3, new wxGridCellFloatEditor() );
            m_padGrid->SetCellValue( nbRow, 4, "-" );
            m_padGrid->SetReadOnly( nbRow, 0 );
            m_padGrid->SetReadOnly( nbRow, 1 );
            m_padGrid->SetReadOnly( nbRow, 4 );
            nbRow++;
        }
    }
    m_padGrid->EnableCellEditControl( true );
    m_padGrid->ShowCellEditControl();
}

void PANEL_PCBNEW_SIMUL_DC_POWER::OnRun( wxCommandEvent& event )
{
    FEM_DESCRIPTOR* descriptor = new FEM_DESCRIPTOR( FEM_SOLVER::SPARSELIZARD, m_board );
    std::cout << m_padGrid->GetNumberRows() << std::endl;
    for( int i = 0; i < m_padGrid->GetNumberRows(); i++ )
    {
        wxString fieldFp = m_padGrid->GetCellValue( i, 0 );
        wxString fieldPad = m_padGrid->GetCellValue( i, 1 );
        wxString fieldType = m_padGrid->GetCellValue( i, 2 );
        wxString fieldValue = m_padGrid->GetCellValue( i, 3 );

        const FOOTPRINT* fp = m_board->FindFootprintByReference( fieldFp );

        if( fp == nullptr )
        {
            std::cerr << "  Could not find footprint " << fieldFp << "." << std::endl;
            continue;
        }

        const PAD* pad = fp->FindPadByName( fieldPad );

        if( pad == nullptr )
        {
            std::cerr << "  Could not find pad " << fieldFp << "-" << fieldPad << "." << std::endl;
            continue;
        }

        FEM_PORT*            port = new FEM_PORT( pad );
        FEM_PORT_CONSTRAINT* constraint = new FEM_PORT_CONSTRAINT();
        std::cout << fieldType << std::endl;
        std::cout << fieldValue << std::endl;
        double valtemp;
        fieldValue.ToDouble( &valtemp );
        std::cout << valtemp << std::endl;

        if( fieldType == "passive" )
        {
            port->m_type = FEM_PORT_TYPE::PASSIVE;
        }
        else if( fieldType == "sink" )
        {
            constraint->m_type = FEM_PORT_CONSTRAINT_TYPE::CURRENT;

            if( fieldValue.ToDouble( &( constraint->m_value ) ) )
                port->m_type = FEM_PORT_TYPE::SINK;
            else
                port->m_type = FEM_PORT_TYPE::PASSIVE;

            constraint->m_value = -constraint->m_value;
        }
        else if( fieldType == "source" )
        {
            constraint->m_type = FEM_PORT_CONSTRAINT_TYPE::VOLTAGE;

            if( fieldValue.ToDouble( &( constraint->m_value ) ) )
                port->m_type = FEM_PORT_TYPE::SOURCE;
            else
                port->m_type = FEM_PORT_TYPE::SINK;
        }
        else
        {
            continue; // ???? unknown port type ?
        }

        port->m_constraint = *constraint;

        FEM_RESULT* result;

        switch( port->m_type )
        {
        case FEM_PORT_TYPE::SINK:
            result = new FEM_RESULT_VALUE( FEM_VALUE_TYPE::VOLTAGE, port, port );
            break;
        case FEM_PORT_TYPE::PASSIVE:
            result = new FEM_RESULT_VALUE( FEM_VALUE_TYPE::VOLTAGE, port, port );
            break;
        case FEM_PORT_TYPE::SOURCE:
            result = new FEM_RESULT_VALUE( FEM_VALUE_TYPE::CURRENT, port, nullptr );
            break;
        default: break;
        }
        descriptor->AddPort( port );
        descriptor->AddResult( result );
        std::cout << "port added 000" << std::endl;
    }

    wxFileName*      file;
    FEM_RESULT_VIEW* view;

    if( m_mapPotential->GetValue() )
    {
        file = new wxFileName( "potential.pos" );
        view = new FEM_RESULT_VIEW( FEM_VIEW_TYPE::VOLTAGE, *file );
        descriptor->AddResult( view );
    }
    if( m_mapCurrentDensity->GetValue() )
    {
        file = new wxFileName( "currentDensity.pos" );
        view = new FEM_RESULT_VIEW( FEM_VIEW_TYPE::CURRENT, *file );
        descriptor->AddResult( view );
    }
    if( m_mapPowerDensity->GetValue() )
    {
        file = new wxFileName( "powerDensity.pos" );
        view = new FEM_RESULT_VIEW( FEM_VIEW_TYPE::POWER, *file );
        descriptor->AddResult( view );
    }

    descriptor->Run();
}