/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2015 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include "map_sizes_grammar.hpp"

namespace mapnik_render
{

void parse_map_sizes(
    map_sizes_grammar<std::string::const_iterator> const & map_sizes_parser,
    std::string const & str,
    std::vector<map_size> & sizes)
{
    boost::spirit::ascii::space_type space;
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    if (!boost::spirit::qi::phrase_parse(iter, end, map_sizes_parser, space, sizes))
    {
        throw std::runtime_error("Failed to parse list of sizes: '" + str + "'");
    }
}

}
