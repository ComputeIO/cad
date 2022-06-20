/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_SPICE_MODEL_H
#define DIALOG_SPICE_MODEL_H

#include <dialog_spice_model_base.h>
#include <netlist_exporter_pspice.h>
#include <scintilla_tricks.h>

#include <sim/sim_model.h>
#include <sch_symbol.h>

// Some probable wxWidgets bugs encountered when writing this class:
// 1. There are rendering problems with wxPropertyGrid on Linux, GTK, Xorg when
//    wxPG_NATIVE_DOUBLE_BUFFERING flag is not set.
// 2. wxPropertyGridManager->ShowHeader() segfaults when called from this dialog's constructor.

template <typename T>
class DIALOG_SPICE_MODEL : public DIALOG_SPICE_MODEL_BASE
{
public:
    enum class COLUMN : int { DESCRIPTION = 0, VALUE, UNIT, DEFAULT, TYPE, END_ };

    DIALOG_SPICE_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol, std::vector<T>& aSchFields );

private:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void updateModel();
    void updateWidgets();
    
    void addParamPropertyIfRelevant( const SIM_MODEL::PARAM& aParam );
    wxPGProperty* newParamProperty( const SIM_MODEL::PARAM& aParam ) const;

    SIM_MODEL& getCurModel();

    void onDeviceTypeChoice( wxCommandEvent& aEvent ) override;
    void onTypeChoice( wxCommandEvent& aEvent ) override;
    //void onPropertyChanged( wxPropertyGridEvent& aEvent ) override;

    SCH_SYMBOL& m_symbol;
    std::vector<T>& m_fields;

    std::vector<std::unique_ptr<SIM_MODEL>> m_models;
    std::map<SIM_MODEL::DEVICE_TYPE, SIM_MODEL::TYPE> m_curModelTypeOfDeviceType;
    SIM_MODEL::TYPE m_curModelType = SIM_MODEL::TYPE::NONE;

    wxPGProperty* m_firstCategory; // Used to add principal parameters to root (any better ideas?)
    std::unique_ptr<SCINTILLA_TRICKS> m_scintillaTricks;
};

#endif /* DIALOG_SPICE_MODEL_H */
