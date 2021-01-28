/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PATHS_H
#define PATHS_H

/**
 * Helper class to centralize the paths used throughout kicad
 */
class PATHS
{
public:
    /**
     * Gets the user path for python scripts
     */
    static wxString GetUserScriptingPath();

    /**
     * Gets the user path for custom templates
     */
    static wxString GetUserTemplatesPath();

    /**
     * Gets the user path for plugins
     */
    static wxString GetUserPluginsPath();

    /**
     * Gets the user path for 3d viewer plugin
     */
    static wxString GetUserPlugins3DPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUserProjectsPath();

    /**
     * Gets the stock (install) scripting path
     */
    static wxString GetStockScriptingPath();

    /**
     * Gets the stock (install) plugins path
     */
    static wxString GetStockPluginsPath();

    /**
     * Gets the stock (install) 3d viewer pluginspath
     */
    static wxString GetStockPlugins3DPath();

    /**
     * Gets the stock (install) 3d viewer pluginspath
     */
    static wxString GetUserCachePath();

private:
    // we are a static helper
    PATHS() {}
};

#endif