/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <panel_common_ui_settings.h>

#include <bitmap_types.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <dpi_scaling.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <id.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>

static constexpr int dpi_scaling_precision = 1;
static constexpr double dpi_scaling_increment = 0.5;


PANEL_COMMON_UI_SETTINGS::PANEL_COMMON_UI_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent )
        : PANEL_COMMON_UI_SETTINGS_BASE( aParent ),
          m_dialog( aDialog ),
          m_last_scale( -1 )
{
    m_canvasScaleCtrl->SetRange(
            DPI_SCALING::GetMinScaleFactor(), DPI_SCALING::GetMaxScaleFactor() );
    m_canvasScaleCtrl->SetDigits( dpi_scaling_precision );
    m_canvasScaleCtrl->SetIncrement( dpi_scaling_increment );
    m_canvasScaleCtrl->SetValue( DPI_SCALING::GetDefaultScaleFactor() );

    m_canvasScaleCtrl->SetToolTip(
            _( "Set the scale for the canvas."
               "\n\n"
               "On high-DPI displays on some platforms, KiCad cannot determine the "
               "scaling factor. In this case you may need to set this to a value to "
               "match your system's DPI scaling. 2.0 is a common value. "
               "\n\n"
               "If this does not match the system DPI scaling, the canvas will "
               "not match the window size and cursor position." ) );

    m_canvasScaleAuto->SetToolTip(
            _( "Use an automatic value for the canvas scale."
               "\n\n"
               "On some platforms, the automatic value is incorrect and should be "
               "set manually." ) );

    m_iconScaleSlider->SetStep( 25 );

    m_canvasScaleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COMMON_UI_SETTINGS::OnCanvasScaleChange ), NULL, this );
}


PANEL_COMMON_UI_SETTINGS::~PANEL_COMMON_UI_SETTINGS()
{
    m_canvasScaleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_COMMON_UI_SETTINGS::OnCanvasScaleChange ), NULL, this );
}


bool PANEL_COMMON_UI_SETTINGS::TransferDataToWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    m_antialiasing->SetSelection( commonSettings->m_Graphics.opengl_aa_mode );
    m_antialiasingFallback->SetSelection( commonSettings->m_Graphics.cairo_aa_mode );

    int icon_scale_fourths = commonSettings->m_Appearance.icon_scale;

    if( icon_scale_fourths <= 0 )
    {
        m_iconScaleAuto->SetValue( true );
        m_iconScaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        m_iconScaleAuto->SetValue( false );
        m_iconScaleSlider->SetValue( icon_scale_fourths * 25 );
    }

    {
        const DPI_SCALING dpi( commonSettings, this );
        m_canvasScaleCtrl->SetValue( dpi.GetScaleFactor() );
        m_canvasScaleAuto->SetValue( dpi.GetCanvasIsAutoScaled() );
    }

    m_checkBoxIconsInMenus->SetValue( commonSettings->m_Appearance.use_icons_in_menus );
    m_ZoomCenterOpt->SetValue( commonSettings->m_Input.center_on_zoom );
    m_MousewheelPANOpt->SetValue( commonSettings->m_Input.mousewheel_pan );
    m_AutoPANOpt->SetValue( commonSettings->m_Input.auto_pan );

    m_PreferSelectToDrag->SetValue( commonSettings->m_Input.prefer_select_to_drag );
    m_warpMouseOnMove->SetValue( commonSettings->m_Input.warp_mouse_on_move );
    m_NonImmediateActions->SetValue( !commonSettings->m_Input.immediate_actions );

    return true;
}


bool PANEL_COMMON_UI_SETTINGS::TransferDataFromWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    commonSettings->m_Graphics.opengl_aa_mode = m_antialiasing->GetSelection();
    commonSettings->m_Graphics.cairo_aa_mode = m_antialiasingFallback->GetSelection();

    const int scale_fourths = m_iconScaleAuto->GetValue() ? -1 : m_iconScaleSlider->GetValue() / 25;
    commonSettings->m_Appearance.icon_scale = scale_fourths;

    {
        DPI_SCALING dpi( commonSettings, this );
        dpi.SetDpiConfig( m_canvasScaleAuto->GetValue(), m_canvasScaleCtrl->GetValue() );
    }

    commonSettings->m_Appearance.use_icons_in_menus = m_checkBoxIconsInMenus->GetValue();

    commonSettings->m_Input.auto_pan = m_AutoPANOpt->GetValue();
    commonSettings->m_Input.center_on_zoom = m_ZoomCenterOpt->GetValue();
    commonSettings->m_Input.immediate_actions = !m_NonImmediateActions->GetValue();
    commonSettings->m_Input.mousewheel_pan = m_MousewheelPANOpt->GetValue();
    commonSettings->m_Input.prefer_select_to_drag = m_PreferSelectToDrag->GetValue();
    commonSettings->m_Input.warp_mouse_on_move = m_warpMouseOnMove->GetValue();

    Pgm().GetSettingsManager().Save( commonSettings );

    return true;
}


void PANEL_COMMON_UI_SETTINGS::OnScaleSlider( wxScrollEvent& aEvent )
{
    m_iconScaleAuto->SetValue( false );
}


void PANEL_COMMON_UI_SETTINGS::OnIconScaleAuto( wxCommandEvent& aEvent )
{
    if( m_iconScaleAuto->GetValue() )
    {
        m_last_scale = m_iconScaleAuto->GetValue();
        m_iconScaleSlider->SetValue( 25 * KiIconScale( GetParent() ) );
    }
    else
    {
        if( m_last_scale >= 0 )
            m_iconScaleSlider->SetValue( m_last_scale );
    }
}


void PANEL_COMMON_UI_SETTINGS::OnCanvasScaleChange( wxCommandEvent& aEvent )
{
    m_canvasScaleAuto->SetValue( false );
}


void PANEL_COMMON_UI_SETTINGS::OnCanvasScaleAuto( wxCommandEvent& aEvent )
{
    const bool automatic = m_canvasScaleAuto->GetValue();

    if( automatic )
    {
        // set the scale to the auto value, without consulting the config
        DPI_SCALING dpi( nullptr, this );

        // update the field (no events sent)
        m_canvasScaleCtrl->SetValue( dpi.GetScaleFactor() );
    }
}
