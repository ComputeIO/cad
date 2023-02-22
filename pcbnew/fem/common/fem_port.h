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

#ifndef FEM_PORT_H
#define FEM_PORT_H

#include <board_connected_item.h>


enum class FEM_PORT_TYPE
{
    SOURCE,
    SINK,
    PASSIVE
};

enum class FEM_PORT_CONSTRAINT_TYPE
{
    VOLTAGE,
    CURRENT,
    POWER,
    CHARGE, // static charge in Coulomb
    SPICE   // unused
};

class FEM_PORT_CONSTRAINT
{
public:
    FEM_PORT_CONSTRAINT_TYPE m_type;
    double                   m_value;
};

class FEM_PORT
{
public:
    FEM_PORT( const BOARD_ITEM* aItem );

    const BOARD_ITEM*           GetItem();
    FEM_PORT_TYPE               m_type;
    FEM_PORT_CONSTRAINT         m_constraint;

private:
    const BOARD_ITEM* m_item;
};

#endif