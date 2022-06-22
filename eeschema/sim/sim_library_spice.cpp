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

#include <sim/sim_library_spice.h>
#include <sim/spice_grammar.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_LIBRARY_SPICE_PARSER
{
    using namespace SPICE_GRAMMAR;

    // TODO: unknownLine is already handled in spiceUnit.
    struct library : spiceSource {};
    struct libraryGrammar : must<library, tao::pegtl::eof> {};


    template <typename Rule> struct librarySelector : std::false_type {};

    template <> struct librarySelector<modelUnit> : std::true_type {};
    template <> struct librarySelector<modelName> : std::true_type {};
    
    // For debugging.
    template <> struct librarySelector<unknownLine> : std::true_type {};
};


void SIM_LIBRARY_SPICE::ReadFile( const wxString& aFilePath )
{
    LOCALE_IO toggle;

    SIM_LIBRARY::ReadFile( aFilePath );

    m_models.clear();
    m_modelNames.clear();

    try
    {
        tao::pegtl::file_input in( aFilePath.ToStdString() );
        auto root = tao::pegtl::parse_tree::parse<SIM_LIBRARY_SPICE_PARSER::libraryGrammar,
                                                  SIM_LIBRARY_SPICE_PARSER::librarySelector>
            ( in );

        wxASSERT( root );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_LIBRARY_SPICE_PARSER::modelUnit>() )
            {
                m_models.push_back( SIM_MODEL::Create( node->string() ) );

                if( node->children.size() < 1
                    || !node->children.at( 0 )->is_type<SIM_LIBRARY_SPICE_PARSER::modelName>() )
                {
                    THROW_IO_ERROR( wxString::Format( "Model name token not found" ) );
                }

                m_modelNames.emplace_back( node->children.at( 0 )->string() );
            }
            else if( node->is_type<SIM_LIBRARY_SPICE_PARSER::unknownLine>() )
            {
                // Do nothing.
            }
            else
            {
                THROW_IO_ERROR( wxString::Format( "Unhandled parse tree node: '%s'",
                                                  node->string() ) );
            }
        }
    }
    catch( const std::filesystem::filesystem_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }
}


void SIM_LIBRARY_SPICE::WriteFile( const wxString& aFilePath )
{
    // Not implemented yet.
}
