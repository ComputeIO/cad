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

#ifndef FEM_DESCRIPTOR_H
#define FEM_DESCRIPTOR_H

#include <board.h>
#include "fem_port.h"
#include "fem_result.h"
#include <list>

#include <board_connected_item.h>

enum class FEM_SIMULATION_TYPE
{
    DC_CURRENT_DENSITY,
    DC_RESISTANCE,
    DC_VOLTAGE_DROP,
    DC_THERMAL
};

enum class FEM_SIMULATION_DIMENSION
{
    SIMUL2D,
    SIMUL2D5,
    SIMUL3D
};

enum class FEM_SOLVER
{
    SPARSELIZARD
};

class FEM_DESCRIPTOR
{
public:
    FEM_DESCRIPTOR( FEM_SOLVER aSolver, const BOARD* aBoard );

    bool Run();

    std::list<FEM_PORT*> GetPorts();
    bool                 AddPort( FEM_PORT* );
    bool                 RemovePort( FEM_PORT* );
    std::list<FEM_RESULT*> GetResults();
    bool                   AddResult( FEM_RESULT* );
    bool                   RemoveResult( FEM_RESULT* );

    FEM_SOLVER GetSolver();
    bool       SetSolver( FEM_SOLVER );

    const BOARD* GetBoard();
    REPORTER*    m_reporter;

    bool                     m_requiresDielectric;
    FEM_SIMULATION_DIMENSION m_dim;

private:
    std::list<FEM_PORT*>   m_ports;
    std::list<FEM_RESULT*> m_results;
    FEM_SOLVER             m_solver;
    const BOARD*           m_board;
};

#endif