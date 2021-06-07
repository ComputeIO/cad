/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fem_descriptor.h"
#include "../sparselizard/sparselizard_solver.h"

FEM_DESCRIPTOR::FEM_DESCRIPTOR( FEM_SOLVER aSolver, const BOARD* aBoard )
{
    m_board = aBoard;
    SetSolver( aSolver );
}

bool FEM_DESCRIPTOR::SetSolver( FEM_SOLVER aSolver )
{
    m_solver = aSolver;
    // When switching solver, we might need to check that all entries are still correct.
    return true;
}

FEM_SOLVER FEM_DESCRIPTOR::GetSolver()
{
    return m_solver;
}

const BOARD* FEM_DESCRIPTOR::GetBoard()
{
    return m_board;
}

std::list<FEM_PORT*> FEM_DESCRIPTOR::GetPorts()
{
    return m_ports;
}

std::list<FEM_RESULT*> FEM_DESCRIPTOR::GetResults()
{
    return m_results;
}

bool FEM_DESCRIPTOR::AddPort( FEM_PORT* aPort )
{
    if( aPort == nullptr )
        return false;
    if( aPort->GetItem() == nullptr )
        return false;

    // Check for unicity
    for( FEM_PORT* port : m_ports )
    {
        if( port->GetItem() == aPort->GetItem() )
            return false;
    }
    m_ports.push_back( aPort );

    return true;
}

bool FEM_DESCRIPTOR::RemovePort( FEM_PORT* aPort )
{
    if( aPort == nullptr )
        return false;
    if( aPort->GetItem() == nullptr )
        return false;

    // @TODO

    return false;
}

bool FEM_DESCRIPTOR::AddResult( FEM_RESULT* aResult )
{
    if( aResult == nullptr )
        return false;
    switch( aResult->GetType() )
    {
    case FEM_RESULT_TYPE::VALUE:
        if( static_cast<FEM_RESULT_VALUE*>( aResult )->IsInitialized() == false )
            return false;
        break;
    case FEM_RESULT_TYPE::VIEW:
        if( static_cast<FEM_RESULT_VIEW*>( aResult )->IsInitialized() == false )
            return false;
        break;
    default: return false;
    }


    // Check for unicity
    for( FEM_RESULT* result : m_results )
    {
        if( result == aResult )
            return false;
    }
    m_results.push_back( aResult );

    return true;
}

bool FEM_DESCRIPTOR::RemoveResult( FEM_RESULT* aResult )
{
    if( aResult == nullptr )
        return false;

    // @TODO

    return false;
}

bool FEM_DESCRIPTOR::Run()
{
    switch( m_solver )
    {
    case FEM_SOLVER::SPARSELIZARD:
    {
        SPARSELIZARD_SOLVER* solver = new SPARSELIZARD_SOLVER( m_reporter );
        return solver->Run_DC( this );
    }
    default: return false;
    }
}