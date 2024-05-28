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
    validateEndpoints();
}

bool HTTP_LIB_V2_CONNECTION::GetPartNames( std::vector<std::string>& aPartNames,
                                           const bool                powerSymbolsOnly )
{
    if( !syncCache() )
    {
        return false;
    }

    for( std::map<std::string, HTTP_LIB_PART>::iterator it = m_parts.begin(); it != m_parts.end();
         ++it )
    {
        if( !powerSymbolsOnly || it->second.PowerSymbol )
        {
            aPartNames.push_back( it->second.Name );
        }
    }

    return true;
}
bool HTTP_LIB_V2_CONNECTION::GetParts( std::vector<HTTP_LIB_PART>& aParts,
                                       const bool                  powerSymbolsOnly )
{
    if( !syncCache() )
    {
        return false;
    }

    for( std::map<std::string, HTTP_LIB_PART>::iterator it = m_parts.begin(); it != m_parts.end();
         ++it )
    {
        if( !powerSymbolsOnly || it->second.PowerSymbol )
        {
            aParts.push_back( it->second );
            it->second.IsLoaded = true;
        }
    }

    return true;
}

bool HTTP_LIB_V2_CONNECTION::GetPart( HTTP_LIB_PART& aPart, const std::string& aPartName,
                                      const bool powerSymbolsOnly )
{
    if( !syncCache() )
    {
        return false;
    }

    aPart = m_parts.at( m_partNameIndex.at( aPartName ) );
    if( !powerSymbolsOnly || aPart.PowerSymbol )
    {
        return true;
    }

    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryNames( std::vector<std::string>& aCategories )
{
    if( !syncCache() )
    {
        return false;
    }

    for( std::map<std::string, HTTP_LIB_V2_CATEGORY>::iterator it = m_categories.begin();
         it != m_categories.end(); ++it )
    {
        aCategories.push_back( it->second.Name );
    }

    return true;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryName( std::string&       aCategoryName,
                                              const std::string& aCategoryId )
{
    if( !syncCache() )
    {
        return false;
    }

    if( m_categories.contains( aCategoryId ) )
    {
        aCategoryName = m_categories.at( aCategoryId ).Name;
        return true;
    }

    m_lastError += fmt::format( "GetCategoryName: Category not found: '{}'\n", aCategoryId );
    return false;
}

bool HTTP_LIB_V2_CONNECTION::GetCategoryDescription( std::string&       aCategoryDescription,
                                                     const std::string& aCategoryName )
{
    if( !syncCache() )
    {
        return false;
    }

    if( m_categoryNameIndex.contains( aCategoryName ) )
    {
        aCategoryDescription =
                m_categories.at( m_categoryNameIndex.at( aCategoryName ) ).Description;
        return true;
    }

    m_lastError +=
            fmt::format( "GetCategoryDescription: Category not found: '{}'\n", aCategoryName );
    return false;
}

void HTTP_LIB_V2_CONNECTION::validateEndpoints()
{
    std::string res;
    m_endpointValid = false;

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    curl->SetURL( m_source.root_url );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
        {
            return;
        }

        if( res.length() == 0 )
        {
            m_lastError = "KiCad received an empty response!\n";
            return;
        }

        nlohmann::json response = nlohmann::json::parse( res );

        if( !response.contains( "version" ) )
        {
            m_lastError = "KiCad received no version from the API server!\n";
            return;
        }

        std::string version = response.at( "version" );

        if( m_supportedApiServerVersions.find( "{" + version + "}" ) == std::string::npos )
        {
            m_lastError = fmt::format(
                    "KiCad does not support the received API server version: '{}'!\n", version );
            return;
        }

        if( response.contains( "content" ) )
        {
            std::string content = response.at( "content" );

            if( !content.empty() )
            {
                size_t pos = content.find( "{}" );
                if( pos == std::string::npos || content.find( "{" ) != pos
                    || content.find( "}" ) != pos + 1
                    || content.find( "{", pos + 2 ) != std::string::npos
                    || content.find( "}", pos + 2 ) != std::string::npos )
                {
                    m_lastError = fmt::format( "KiCad does not support the received API server "
                                               "content endpoint: '{}'!\n",
                                               content );
                    return;
                }
                m_contentEndpoint = content;
            }
        }
        m_endpointValid = true;
    }
    catch( const std::exception& e )
    {
        m_lastError = fmt::format( "Error: {}\nAPI Response:  {}\n", e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "ValidateHTTPLibraryEndpoints: Exception occurred while testing the API "
                         "connection: %s" ),
                    m_lastError );

        m_endpointValid = false;
    }
}

bool HTTP_LIB_V2_CONNECTION::syncCache()
{
    if( !m_endpointValid )
    {
        wxLogTrace( traceHTTPLib, wxT( "syncCache: without valid connection!" ) );
        return false;
    }

    std::string res;

    if( std::difftime( std::time( nullptr ), m_lastCached ) > m_source.timeout_cache )
    {
        m_lastCached = std::time( nullptr );

        std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();

        curl->SetURL( m_source.root_url
                      + fmt::vformat( m_contentEndpoint, fmt::make_format_args( m_timestamp ) ) );

        m_timestamp = 0;
        try
        {
            curl->Perform();

            res = curl->GetBuffer();

            if( !checkServerResponse( curl ) )
            {
                return false;
            }

            if( res.length() == 0 )
            {
                m_lastError = "KiCad received an empty response!\n";
                return false;
            }

            nlohmann::ordered_json response = nlohmann::ordered_json::parse( res );

            if( response.contains( "timestamp" ) )
            {
                m_timestamp = response.at( "timestamp" );
            }

            if( response.contains( "categories" ) )
            {
                for( const auto& category : response.at( "categories" ).items() )
                {
                    readCategory( category.value() );
                }
            }

            if( response.contains( "parts" ) )
            {
                for( const auto& part : response.at( "parts" ).items() )
                {
                    readPart( part.value() );
                }
            }
            return true;
        }
        catch( const std::exception& e )
        {
            m_lastError = fmt::format( "Error: {}\nAPI Response:  {}\n", e.what(), res );

            wxLogTrace( traceHTTPLib,
                        wxT( "Exception occurred while syncing parts from REST API: %s" ),
                        m_lastError );

            return false;
        }
    }
    return true;
}

void HTTP_LIB_V2_CONNECTION::readCategory( const nlohmann::json& aCategory )
{
    std::string id = aCategory.at( "id" );

    m_categoryNameIndex.erase( m_categories[id].Name );

    if( aCategory.contains( "active" ) )
    {
        bool active = aCategory.at( "active" );
        if( !active )
        {
            std::vector<std::string> obsoletParts;

            for( std::map<std::string, HTTP_LIB_PART>::iterator it = m_parts.begin();
                 it != m_parts.end(); ++it )
            {
                if( it->second.CategoryId == id )
                {
                    obsoletParts.push_back( it->second.Id );
                }
            }

            for( auto& it : obsoletParts )
            {
                m_partNameIndex.erase( m_parts[it].Name );
                m_parts.erase( it );
            }

            m_categories.erase( id );
            return;
        }
    }

    HTTP_LIB_V2_CATEGORY category = HTTP_LIB_V2_CATEGORY( id );

    if( aCategory.contains( "name" ) )
    {
        category.Name = aCategory.at( "name" );
    }
    else
    {
        category.Name = id;
    }

    if( aCategory.contains( "description" ) )
    {
        category.Description = aCategory.at( "description" );
    }

    m_categoryNameIndex[category.Name] = id;
    m_categories[id] = category;
}

void HTTP_LIB_V2_CONNECTION::readPart( const nlohmann::ordered_json& aPart )
{
    std::string id = aPart.at( "id" );
    std::string categoryId = aPart.at( "category_id" );

    m_partNameIndex.erase( m_parts[id].Name );

    if( aPart.contains( "active" ) )
    {
        bool active = aPart.at( "active" );
        if( !active )
        {
            m_parts.erase( id );
            return;
        }
    }

    HTTP_LIB_PART part = HTTP_LIB_PART( id, categoryId );

    if( aPart.contains( "name" ) )
    {
        part.Name = aPart.at( "name" );
    }
    else
    {
        part.Name = id;
    }

    part.Symbol = aPart.at( "symbol" );

    if( aPart.contains( "keywords" ) )
    {
        part.Keywords = aPart.at( "keywords" );
    }

    if( aPart.contains( "reference" ) )
    {
        readField( part.Reference, aPart.at( "reference" ) );
    }

    if( aPart.contains( "value" ) )
    {
        readField( part.Value, aPart.at( "value" ) );
    }

    if( aPart.contains( "footprint" ) )
    {
        readField( part.Footprint, aPart.at( "footprint" ) );
    }

    if( aPart.contains( "datasheet" ) )
    {
        readField( part.Datasheet, aPart.at( "datasheet" ) );
    }

    if( aPart.contains( "description" ) )
    {
        readField( part.Description, aPart.at( "description" ) );
    }

    if( aPart.contains( "fields" ) )
    {
        std::string fieldName;

        for( const auto& jsonField : aPart.at( "fields" ).items() )
        {
            fieldName = jsonField.key();

            HTTP_LIB_FIELD field = HTTP_LIB_FIELD();
            readField( field, jsonField.value() );

            part.Fields.push_back( std::make_pair( fieldName, field ) );
        }
    }

    if( aPart.contains( "exclude_from_bom" ) )
    {
        part.ExcludeFromBom = aPart.at( "exclude_from_bom" );
    }

    if( aPart.contains( "exclude_from_board" ) )
    {
        part.ExcludeFromBoard = aPart.at( "exclude_from_board" );
    }

    if( aPart.contains( "exclude_from_sim" ) )
    {
        part.ExcludeFromSim = aPart.at( "exclude_from_sim" );
    }

    if( aPart.contains( "power_symbol" ) )
    {
        part.PowerSymbol = aPart.at( "power_symbol" );
    }

    m_partNameIndex[part.Name] = id;
    m_parts[id] = part;
}

void HTTP_LIB_V2_CONNECTION::readField( HTTP_LIB_FIELD& aField, const nlohmann::json& aJsonField )
{
    if( aJsonField.contains( "value" ) )
    {
        aField.Value = aJsonField.at( "value" );
    }
    if( aJsonField.contains( "show" ) )
    {
        aField.Show = aJsonField.at( "show" );
    }
    if( aJsonField.contains( "show_name" ) )
    {
        aField.ShowName = aJsonField.at( "show_name" );
    }
}