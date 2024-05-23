/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
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

#ifndef KICAD_HTTP_LIB_V2_CONNECTION_H
#define KICAD_HTTP_LIB_V2_CONNECTION_H

#include <any>
#include <boost/algorithm/string.hpp>

#include "http_lib/http_lib_settings.h"
#include <kicad_curl/kicad_curl_easy.h>
#include <http_lib/http_lib_connection.h>

class HTTP_LIB_V2_CONNECTION : public HTTP_LIB_CONNECTION
{
public:
    HTTP_LIB_V2_CONNECTION( const HTTP_LIB_SOURCE& aSource );

    ~HTTP_LIB_V2_CONNECTION() {}

    bool GetPartNames( std::vector<std::string>& aPartNames, const bool powerSymbolsOnly ) override
    {
        return false;
    }

    bool GetParts( std::vector<HTTP_LIB_PART>& aParts, const bool powerSymbolsOnly ) override
    {
        return false;
    }

    bool GetPart( HTTP_LIB_PART& aPart, const std::string& aPartName, const bool powerSymbolsOnly )
    {
        return false;
    }

    bool GetCategoryNames( std::vector<std::string>& aCategories ) override { return false; }

    bool GetCategoryName( std::string& aCategoryName, const std::string& aCategoryId ) override
    {
        return false;
    }

    bool GetCategoryDescription( std::string&       aCategoryDescription,
                                 const std::string& aCategoryName ) override
    {
        return false;
    }

private:
};

#endif //KICAD_HTTP_LIB_V2_CONNECTION_H
