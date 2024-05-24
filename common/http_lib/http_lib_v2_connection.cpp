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

#include <wx/log.h>
#include <fmt/core.h>
#include <wx/translation.h>
#include <ctime>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <wx/base64.h>

#include <kicad_curl/kicad_curl_easy.h>
#include <curl/curl.h>

#include <http_lib/http_lib_connection.h>
#include <http_lib/HTTP_LIB_V2_CONNECTION.h>

HTTP_LIB_V2_CONNECTION::HTTP_LIB_V2_CONNECTION( const HTTP_LIB_SOURCE& aSource ) :
        HTTP_LIB_CONNECTION( aSource )
{
}

bool HTTP_LIB_V2_CONNECTION::GetPartNames( std::vector<std::string>& aPartNames,
                                           const bool                powerSymbolsOnly )
{
    m_lastError += "GetPartNames: not implemented!";
    return false;
}
bool HTTP_LIB_V2_CONNECTION::GetParts( std::vector<HTTP_LIB_PART>& aParts,
                                       const bool                  powerSymbolsOnly )
{
    m_lastError += "GetParts: not implemented!";
    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetPart( HTTP_LIB_PART& aPart, const std::string& aPartName,
                                      const bool powerSymbolsOnly )
{
    m_lastError += "GetPart: not implemented!";
    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryNames( std::vector<std::string>& aCategories )
{
    m_lastError += "GetCategoryNames: not implemented!";
    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryName( std::string&       aCategoryName,
                                              const std::string& aCategoryId )
{
    m_lastError += "GetCategoryName: not implemented!";
    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryDescription( std::string&       aCategoryDescription,
                                                     const std::string& aCategoryName )
{
    m_lastError += "GetCategoryDescription: not implemented!";
    return false;
}