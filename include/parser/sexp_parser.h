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

#ifndef SEXP_PARSER_H
#define SEXP_PARSER_H

#include <pegtl.hpp>
#include <iostream>
#include <string>
#include <utf8.h>

namespace SEXP
{
using namespace tao::pegtl;

struct SEXP_NODE : parse_tree::basic_node<SEXP_NODE>
{
};

/*
 * Grammar
 */
// clang-format off
struct HASH_COMMENT : until<eolf> {};

struct LIST;

struct LIST_COMMENT : if_must<at<one<'('>>, disable<LIST>>{};

//struct READ_INCLUDE : seq< one< ' ' >, one< '"' >, plus< not_one< '"' > >, one< '"' > > {};
//struct HASH_INCLUDE : if_must< string< 'i', 'n', 'c', 'l', 'u', 'd', 'e' >, read_include > {};

//struct HASHED : if_must< one< '#' >, sor< hash_include, list_comment, hash_comment > > {};

struct HASHED : if_must<one<'#'>, sor<LIST_COMMENT, HASH_COMMENT>> {};

struct NUMBER : plus<digit> {};

struct SYMBOL : identifier {};

struct ATOM : sor<NUMBER, SYMBOL> {};

struct ANYTHING;

struct STRING : if_must<one<'"'>, until<one<'"'>, ANYTHING>> {};

struct LIST : if_must<one<'('>, until<one<')'>, ANYTHING>> {};

struct NORMAL : sor<ATOM, LIST> {};

struct ANYTHING : sor<space, HASHED, NORMAL> {};

struct GRAMMAR : until<eof, must<ANYTHING>> {};
// clang-format on

template <typename Rule>
using SELECTOR = parse_tree::selector<Rule, parse_tree::store_content::on<NORMAL>>;

typedef std::unique_ptr<SEXP::SEXP_NODE, std::default_delete<SEXP::SEXP_NODE>> NODE;
//typedef std::unique_ptr<SEXP::SEXP_NODE> NODE;

class SEXP_PARSER
{
public:
    SEXP_PARSER( const std::string& source ) : in( source, "from_input" ) {}

    NODE parse();

private:
    string_input<> in;
};

} // namespace SEXP

#endif //SEXP_PARSER_H
