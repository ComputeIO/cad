/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _COMMON_SETTINGS_H
#define _COMMON_SETTINGS_H

#include <settings/json_settings.h>


class COMMON_SETTINGS : public JSON_SETTINGS
{
public:
    struct APPEARANCE
    {
        double canvas_scale;
        int icon_scale;
        bool use_icons_in_menus;
    };

    struct AUTO_BACKUP
    {
        bool   enabled;            ///< Automatically back up the project when files are saved
        bool   backup_on_autosave; ///< Trigger a backup on autosave
        int    limit_total_files;  ///< Maximum number of backup archives to retain
        int    limit_daily_files;  ///< Maximum files to keep per day, 0 for unlimited
        int    min_interval;       ///< Minimum time, in seconds, between subsequent backups

        /// Maximum total size of backups (bytes), 0 for unlimited
        unsigned long long limit_total_size;
    };

    struct ENVIRONMENT
    {
        bool show_warning_dialog;
        std::map<std::string, wxString> vars;
    };

    struct INPUT
    {
        bool auto_pan;
        int  auto_pan_acceleration;
        bool center_on_zoom;
        bool immediate_actions;
        bool prefer_select_to_drag;
        bool warp_mouse_on_move;
        bool horizontal_pan;

        bool zoom_acceleration;
        int  zoom_speed;
        bool zoom_speed_auto;

        int scroll_modifier_zoom;
        int scroll_modifier_pan_h;
        int scroll_modifier_pan_v;

        int drag_middle;
        int drag_right;
    };

    struct GRAPHICS
    {
        int cairo_aa_mode;
        int opengl_aa_mode;
    };

    struct SYSTEM
    {
        int autosave_interval;
        wxString editor_name;
        int file_history_size;
        wxString language;
        wxString pdf_viewer_name;
        bool use_system_pdf_viewer;
        wxString working_dir;
    };

    COMMON_SETTINGS();

    virtual ~COMMON_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    bool Migrate() override;

private:
    bool migrateSchema0to1();

public:
    APPEARANCE m_Appearance;

    AUTO_BACKUP m_Backup;

    ENVIRONMENT m_Env;

    INPUT m_Input;

    GRAPHICS m_Graphics;

    SYSTEM m_System;

    // TODO: These may not want to be in common
    wxString m_3DLibsUrl;

    wxString m_3DLibsDownloadPath;
};

#endif
