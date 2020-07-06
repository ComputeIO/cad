/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Reece R. Pollack <reece@his.com>
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

#include <wx/gdicmn.h>
#include <pcb_base_frame.h>
#include <pcb_display_options.h>
#include <pcb_origin_transforms.h>

PCB_ORIGIN_TRANSFORMS::PCB_ORIGIN_TRANSFORMS( PCB_BASE_FRAME& aPcbBaseFrame )
        : m_pcbBaseFrame( aPcbBaseFrame ),
          m_invertXAxis( aPcbBaseFrame.GetDisplayOptions().m_DisplayInvertXAxis ),
          m_invertYAxis( aPcbBaseFrame.GetDisplayOptions().m_DisplayInvertYAxis )
{}

PCB_ORIGIN_TRANSFORMS::~PCB_ORIGIN_TRANSFORMS()
{}

int PCB_ORIGIN_TRANSFORMS::GetUserXOrigin() const
{
    return m_pcbBaseFrame.GetUserOrigin().x;
}

int PCB_ORIGIN_TRANSFORMS::GetUserYOrigin() const
{
    return m_pcbBaseFrame.GetUserOrigin().y;
}

