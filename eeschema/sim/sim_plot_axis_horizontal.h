/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SIM_PLOT_AXIS_HORIZONTAL_H
#define __SIM_PLOT_AXIS_HORIZONTAL_H

#include "sim_plot_axis_horizontal_base.h"
#include <wx/string.h>


class SIM_PLOT_AXIS_HORIZONTAL : public SIM_PLOT_AXIS_HORIZONTAL_BASE
{
public:
    SIM_PLOT_AXIS_HORIZONTAL( wxWindow* aParent, wxString quantity, double left, double tick,
                              double right, bool log );


    virtual ~SIM_PLOT_AXIS_HORIZONTAL(){};

    double   m_left = 0;
    double   m_ticks = 0;
    double   m_right = 0;
    bool     m_log = false;
    wxString m_quantity;


    virtual void OnOKButtonClick( wxCommandEvent& event ) override;
};

#endif
