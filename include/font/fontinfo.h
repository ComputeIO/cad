/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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

#ifndef FONT_FONTINFO_H
#define FONT_FONTINFO_H

#include <string>
#include <vector>
namespace fontconfig
{
class FONTINFO
{
public:
    FONTINFO( std::string aFile, std::string aStyle, std::string aFamily ) :
            mFile( aFile ), mStyle( aStyle ), mFamily( aFamily )
    {
    }

    const std::string& File() { return mFile; }

    const std::string& Style() { return mStyle; }

    const std::string& Family() { return mFamily; }

    std::vector<FONTINFO>& Children() { return children; }

private:
    std::string mFile;
    std::string mStyle;
    std::string mFamily;

    std::vector<FONTINFO> children;
};
} // namespace fontconfig

#endif //FONT_FONTINFO_H
