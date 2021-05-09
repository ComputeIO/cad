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

#include "fem_result.h"

FEM_RESULT::FEM_RESULT( FEM_RESULT_TYPE aType )
{
    m_type = aType;
    m_valid = false;
    m_initialized = false;
}

FEM_RESULT_VALUE::FEM_RESULT_VALUE( FEM_VALUE_TYPE aType, FEM_PORT* aPortA, FEM_PORT* aPortB ) :
        FEM_RESULT( FEM_RESULT_TYPE::VALUE )
{
    if( aPortA == nullptr )
    {
        std::cerr << "FEM_RESULT_VALUE : Port A is unspecificed." << std::endl;
    }
    if( ( aType >= FEM_VALUE_TYPE::REQUIRES_2_PORTS ) && ( aPortB == nullptr ) )
    {
        std::cerr << "FEM_RESULT_VALUE : Port B is unspecificed for value requiring 2 ports."
                  << std::endl;
    }
    if( ( aType >= FEM_VALUE_TYPE::REQUIRES_2_PORTS )
        && ( aPortA->GetItem() == aPortB->GetItem() ) )
    {
        std::cerr << "FEM_RESULT_VALUE : Port A and Port B are the same item." << std::endl;
    }

    m_valueType = aType;
    m_portA = aPortA;
    m_portB = aPortB;
    m_initialized = true;
}