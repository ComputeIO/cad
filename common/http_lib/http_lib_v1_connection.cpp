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
#include <http_lib/http_lib_v1_connection.h>

HTTP_LIB_V1_CONNECTION::HTTP_LIB_V1_CONNECTION( const HTTP_LIB_SOURCE& aSource ) :
        HTTP_LIB_CONNECTION( aSource )
{
    validateHTTPLibraryEndpoints();
    m_endpointValid = syncCategories();
}

bool HTTP_LIB_V1_CONNECTION::GetPartNames( std::vector<std::string>& aPartNames,
                                           const bool                powerSymbolsOnly )
{
    if( !syncParts() )
    {
        return false;
    }

    std::vector<std::string> obsolet_parts;

    for( std::map<std::string, HTTP_LIB_V1_PART>::iterator it = m_parts.begin();
         it != m_parts.end(); ++it )
    {
        if( it->second.outdated && m_categories.at( it->second.CategoryId ).check_for_outdated )
        {
            obsolet_parts.push_back( it->first );
        }
        else
        {
            aPartNames.push_back( it->second.Name );
            it->second.outdated = true;
        }
    }

    for( auto& it : obsolet_parts )
    {
        m_parts.erase( it );
    }

    for( std::map<std::string, HTTP_LIB_V1_CATEGORY>::iterator it = m_categories.begin();
         it != m_categories.end(); ++it )
    {
        it->second.check_for_outdated = false;
    }

    return true;
}

bool HTTP_LIB_V1_CONNECTION::GetParts( std::vector<HTTP_LIB_PART>& aParts,
                                       const bool                  powerSymbolsOnly )
{
    if( !syncParts() )
    {
        return false;
    }

    std::vector<std::string> obsolet_parts;

    for( std::map<std::string, HTTP_LIB_V1_PART>::iterator it = m_parts.begin();
         it != m_parts.end(); ++it )
    {
        if( it->second.outdated && m_categories.at( it->second.CategoryId ).check_for_outdated )
        {
            obsolet_parts.push_back( it->first );
        }
        else
        {
            aParts.push_back( it->second );
            it->second.outdated = true;
        }
    }

    for( auto& it : obsolet_parts )
    {
        m_parts.erase( it );
    }

    for( std::map<std::string, HTTP_LIB_V1_CATEGORY>::iterator it = m_categories.begin();
         it != m_categories.end(); ++it )
    {
        it->second.check_for_outdated = false;
    }

    return true;
}

bool HTTP_LIB_V1_CONNECTION::GetPart( HTTP_LIB_PART& aPart, const std::string& aPartName,
                                      const bool powerSymbolsOnly )
{
    if( !syncParts() )
    {
        return false;
    }

    if( !m_partNameIndex.contains( aPartName ) )
    {
        m_lastError += wxString::Format( _( "GetPart: Part not found: '%s'" ) + "\n", aPartName );
        return false;
    }

    HTTP_LIB_V1_PART part = m_parts.at( m_partNameIndex.at( aPartName ) );

    if( std::difftime( std::time( nullptr ), part.lastCached ) > m_source.timeout_parts )
    {
        if( cacheUpdateOne( part ) )
        {
            m_parts[m_partNameIndex.at( aPartName )] = part;
        }
        else
        {
            return false;
        }
    }
    aPart = part;
    return true;
}

bool HTTP_LIB_V1_CONNECTION::GetCategoryNames( std::vector<std::string>& aCategories )
{
    for( std::map<std::string, HTTP_LIB_V1_CATEGORY>::iterator it = m_categories.begin();
         it != m_categories.end(); ++it )
    {
        aCategories.push_back( it->second.name );
    }
    return true;
}

bool HTTP_LIB_V1_CONNECTION::GetCategoryName( std::string&       aCategoryName,
                                              const std::string& aCategoryId )
{
    if( m_categories.contains( aCategoryId ) )
    {
        aCategoryName = m_categories.at( aCategoryId ).name;
        return true;
    }
    m_lastError +=
            wxString::Format( _( "GetCategoryName: Category found: '%s'" ) + "\n", aCategoryId );
    return false;
}

bool HTTP_LIB_V1_CONNECTION::GetCategoryDescription( std::string&       aCategoryDescription,
                                                     const std::string& aCategoryName )
{
    if( m_categoryNameIndex.contains( aCategoryName ) )
    {
        aCategoryDescription =
                m_categories.at( m_categoryNameIndex.at( aCategoryName ) ).description;
        return true;
    }

    m_lastError += wxString::Format( _( "GetCategoryDescription: Category found: '%s'" ) + "\n",
                                     aCategoryName );
    return false;
}

bool HTTP_LIB_V1_CONNECTION::validateHTTPLibraryEndpoints()
{
    m_endpointValid = false;
    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    curl->SetURL( m_source.root_url );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
            return false;

        if( res.length() == 0 )
        {
            m_lastError += wxString::Format( _( "KiCad received an empty response!" ) + "\n" );
        }
        else
        {
            nlohmann::json response = nlohmann::json::parse( res );

            // Check that the endpoints exist, if not fail.
            if( !response.at( http_endpoint_categories ).empty()
                && !response.at( http_endpoint_parts ).empty() )
            {
                m_endpointValid = true;
            }
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "ValidateHTTPLibraryEndpoints: Exception occurred while testing the API "
                         "connection: %s" ),
                    m_lastError );

        m_endpointValid = false;
    }
    return m_endpointValid;
}

bool HTTP_LIB_V1_CONNECTION::syncCategories()
{
    if( !m_endpointValid )
    {
        wxLogTrace( traceHTTPLib, wxT( "syncCategories: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    curl->SetURL( m_source.root_url + http_endpoint_categories + ".json" );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
            return false;

        nlohmann::json response = nlohmann::json::parse( res );

        // collect the categories in dictionary
        for( const auto& item : response.items() )
        {
            HTTP_LIB_V1_CATEGORY category;

            auto& value = item.value();
            category.id = value["id"].get<std::string>();
            category.name = value["name"].get<std::string>();

            if( value.contains( "description" ) )
            {
                category.description = value["description"].get<std::string>();
            }

            m_categoryNameIndex[category.name] = category.id;
            m_categories[category.id] = category;
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "syncCategories: Exception occurred while syncing categories: %s" ),
                    m_lastError );

        m_categories.clear();
        return false;
    }
    return true;
}

bool HTTP_LIB_V1_CONNECTION::syncParts()
{
    for( std::map<std::string, HTTP_LIB_V1_CATEGORY>::iterator it = m_categories.begin();
         it != m_categories.end(); ++it )
    {
        if( std::difftime( std::time( nullptr ), it->second.lastCached )
            > m_source.timeout_categories )
        {
            if( !cacheAll( it->second ) )
            {
                return false;
            }
            it->second.check_for_outdated = true;
        }
    }
}

bool HTTP_LIB_V1_CONNECTION::cacheAll( const HTTP_LIB_V1_CATEGORY& aCategory )
{
    if( !m_endpointValid )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectAll: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();

    curl->SetURL( m_source.root_url
                  + fmt::format( "{}/category/{}.json", http_endpoint_parts, aCategory.id ) );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        nlohmann::json response = nlohmann::json::parse( res );

        for( nlohmann::json& item : response )
        {
            std::string id = item.at( "id" );

            HTTP_LIB_V1_PART part = m_parts[id];

            part.Id = id;
            part.CategoryId = aCategory.id;
            part.preloadedDescription = false;

            if( m_partNameIndex.contains( part.Name ) )
            {
                m_partNameIndex.erase( part.Name );
            }

            // API might not want to return an optional name.
            if( item.contains( "name" ) )
            {
                part.Name = item.at( "name" );
            }
            else
            {
                part.Name = id;
            }

            // API might not want to return an optional description.
            if( item.contains( description_field ) )
            {
                part.Description.Value = item.at( description_field );
                part.preloadedDescription = true;
            }

            if( item.contains( "power_symbol" ) )
            {
                std::string pow = item.at( "power_symbol" );
                part.PowerSymbol = boolFromString( pow, false );
            }

            part.outdated = false;

            m_partNameIndex[part.Name] = id;
            m_parts[part.Id] = part;
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib, wxT( "Exception occurred while syncing parts from REST API: %s" ),
                    m_lastError );

        return false;
    }

    return true;
}

bool HTTP_LIB_V1_CONNECTION::cacheUpdateOne( HTTP_LIB_V1_PART& aPart )
{
    if( !m_endpointValid )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectAll: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    std::string                      url =
            m_source.root_url + fmt::format( "{}/{}.json", http_endpoint_parts, aPart.Id );
    curl->SetURL( url );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
        {
            return false;
        }

        nlohmann::ordered_json response = nlohmann::ordered_json::parse( res );
        std::string            key = "";
        std::string            value = "";

        // get a timestamp for caching
        aPart.lastCached = std::time( nullptr );

        aPart.Symbol = response.at( "symbolIdStr" );

        // initially assume no exclusion
        std::string exclude;

        if( response.contains( "exclude_from_bom" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_bom" );
            aPart.ExcludeFromBom = boolFromString( exclude, false );
        }

        // initially assume no exclusion
        if( response.contains( "exclude_from_board" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_board" );
            aPart.ExcludeFromBoard = boolFromString( exclude, false );
        }

        // initially assume no exclusion
        if( response.contains( "exclude_from_sim" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_sim" );
            aPart.ExcludeFromSim = boolFromString( exclude, false );
        }

        //        aPart.Fields[]

        // remove previously loaded fields
        aPart.Fields.clear();

        // Extract available fields
        for( const auto& field : response.at( "fields" ).items() )
        {
            bool visible = true;

            // name of the field
            key = field.key();
            wxString fieldName = wxString( key ).Lower();

            // this is a dict
            auto& properties = field.value();

            value = properties.at( "value" );

            // check if user wants to display field in schematic
            if( properties.contains( "visible" ) )
            {
                std::string vis = properties.at( "visible" );
                visible = boolFromString( vis, true );
            }

            if( fieldName == footprint_field )
            {
                aPart.Footprint.Value = value;
                aPart.Footprint.Show = visible;
            }
            else if( fieldName == description_field )
            {
                if( !aPart.preloadedDescription )
                {
                    aPart.Description.Value = value;
                }
                aPart.Description.Show = visible;
            }
            else if( fieldName == value_field )
            {
                aPart.Value.Value = value;
                aPart.Value.Show = visible;
            }
            else if( fieldName == datasheet_field )
            {
                aPart.Datasheet.Value = value;
                aPart.Datasheet.Show = visible;
            }
            else if( fieldName == reference_field )
            {
                aPart.Reference.Value = value;
                aPart.Reference.Show = visible;
            }
            else if( fieldName == keywords_field )
            {
                aPart.Keywords = value;
            }
            else
            {
                // Add field to fields list
                aPart.Fields.push_back( std::make_pair( key, HTTP_LIB_FIELD( value, visible ) ) );
            }
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "SelectOne: Exception occurred while retrieving part from REST API: %s" ),
                    m_lastError );

        return false;
    }

    return true;
}

bool HTTP_LIB_V1_CONNECTION::boolFromString( const std::any& aVal, bool aDefaultValue )
{
    try
    {
        wxString strval( std::any_cast<std::string>( aVal ).c_str(), wxConvUTF8 );

        if( strval.IsEmpty() )
            return aDefaultValue;

        strval.MakeLower();

        for( const auto& trueVal : { wxS( "true" ), wxS( "yes" ), wxS( "y" ), wxS( "1" ) } )
        {
            if( strval.Matches( trueVal ) )
                return true;
        }

        for( const auto& falseVal : { wxS( "false" ), wxS( "no" ), wxS( "n" ), wxS( "0" ) } )
        {
            if( strval.Matches( falseVal ) )
                return false;
        }
    }
    catch( const std::bad_any_cast& )
    {
    }

    return aDefaultValue;
}