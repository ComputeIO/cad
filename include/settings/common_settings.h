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

    struct ENVIRONMENT
    {
        bool show_warning_dialog;
        std::map<std::string, wxString> vars;
    };

    struct INPUT
    {
        bool auto_pan;
        bool center_on_zoom;
        bool immediate_actions;
        bool mousewheel_pan;
        bool prefer_select_to_drag;
        bool warp_mouse_on_move;
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

    struct FILE_HANDLING
    {
        int  compressionLevel;
        bool preferCompressedPcbFiles;
    };

    COMMON_SETTINGS();

    virtual ~COMMON_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

public:
    APPEARANCE m_Appearance;

    ENVIRONMENT m_Env;

    INPUT m_Input;

    GRAPHICS m_Graphics;

    SYSTEM m_System;

    FILE_HANDLING m_FileHandling;

    // TODO: These may not want to be in common
    wxString m_3DLibsUrl;

    wxString m_3DLibsDownloadPath;
};

#endif
