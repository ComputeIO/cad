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

#ifndef MARKUP_PARSER_H
#define MARKUP_PARSER_H

#include <pegtl.hpp>
#include <iostream>
#include <string>
#include <utf8.h>

/****************************************************************
 * markup
 ****************************************************************/
namespace MARKUP
{
using namespace tao::pegtl;

struct NODE : parse_tree::basic_node<NODE>
{
    std::string asString() const;

    std::string typeString() const;

    bool isOverbar() const;

    bool isSubscript() const;

    bool isSuperscript() const;
};

struct dollar : string<'$'>
{
};

// two consecutive dollar signs ( -> a dollar sign )
struct quotedDollar : seq<string<'$'>, dollar>
{
};

struct tilde : string<'~'>
{
};

// two consecutive tildes ( -> a tilde )
struct quotedTilde : seq<string<'~'>, tilde>
{
};

struct varPrefix : string<'$', '{'>
{
};

struct subPrefix : string<'_', '{'>
{
};

struct supPrefix : string<'^', '{'>
{
};

struct closeBrace : string<'}'>
{
};

struct varName : plus<sor<identifier_other, string<' '>>>
{
};

struct varNamespaceName : plus<identifier>
{
};

struct varNamespace : seq<varNamespaceName, string<':'>>
{
};

struct variable : seq<varPrefix, opt<varNamespace>, varName, closeBrace>
{
};

struct subscript;
struct superscript;

/**
 * anyString =
 * a run of characters that do not start a command sequence,
 * or if they do, they do not start a complete command prefix
 * (command char + open brace) - note that '$' is considered
 * special even if it is not followed by an open brace
 */
struct anyString : plus<sor<utf8::not_one<'~', '$', '_', '^'>, seq<not_at<subPrefix>, string<'_'>>,
                            seq<not_at<supPrefix>, string<'^'>>>>
{
};

struct prefixedSuperscript : seq<supPrefix, superscript>
{
};

struct prefixedSubscript : seq<subPrefix, subscript>
{
};

struct anyStringWithinBraces : plus<sor<utf8::not_one<'~', '$', '_', '^', '}'>>>
{
};

struct superscript
        : until<closeBrace, sor<variable, quotedTilde, quotedDollar, anyStringWithinBraces>>
{
};

struct subscript
        : until<closeBrace, sor<variable, quotedTilde, quotedDollar, anyStringWithinBraces>>
{
};

struct notSubOrSup : seq<not_at<subPrefix>, not_at<supPrefix>>
{
};


struct tildeString : plus<seq<not_at<subPrefix>, identifier_other>>
{
};

/**
 * Finally, the full grammar
 *
 */
struct grammar : star<sor<quotedDollar, quotedTilde, variable, prefixedSubscript,
                          prefixedSuperscript, seq<string<'~'>, tildeString>, anyString>>
{
};

template <typename Rule>
using selector = parse_tree::selector<
        Rule,
        parse_tree::store_content::on<dollar, tilde, varNamespaceName, varName, tildeString,
                                      anyString, anyStringWithinBraces>,
        parse_tree::discard_empty::on<superscript, subscript>>;

typedef std::unique_ptr<MARKUP::NODE, std::default_delete<MARKUP::NODE>> MARKUP_NODE;

class MARKUP_PARSER
{
public:
    MARKUP_PARSER( const std::string& source ) : in( source, "from_input" ) {}

    MARKUP_NODE Parse();

private:
    string_input<> in;
};
} // namespace MARKUP

std::ostream& operator<<( std::ostream& os, const MARKUP::MARKUP_NODE& node );

#endif //MARKUP_PARSER_H
