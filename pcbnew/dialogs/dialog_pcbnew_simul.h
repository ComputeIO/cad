/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef KICAD_DIALOG_PCBNEW_SIMUL_H
#define KICAD_DIALOG_PCBNEW_SIMUL_H

#include <widgets/paged_dialog.h>

class PCB_EDIT_FRAME;
class PANEL_PCBNEW_SIMUL_DC_POWER;


class DIALOG_PCBNEW_SIMUL : public PAGED_DIALOG
{
public:
    DIALOG_PCBNEW_SIMUL( PCB_EDIT_FRAME* aFrame );
    ~DIALOG_PCBNEW_SIMUL();

protected:
    void OnAuxiliaryAction( wxCommandEvent& event ) override;

    PCB_EDIT_FRAME* m_frame;

    PANEL_PCBNEW_SIMUL_DC_POWER* m_DCpower;

    // event handlers
    void OnPageChange( wxBookCtrlEvent& event );

private:
};


#endif //KICAD_DIALOG_PCBNEW_SIMUL_H
