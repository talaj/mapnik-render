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

#ifndef MAPNIK_RENDER_RUNNER_HPP
#define MAPNIK_RENDER_RUNNER_HPP

#include "config.hpp"
#include "report.hpp"
#include "renderer.hpp"
#include "map_sizes_grammar.hpp"

namespace mapnik_render
{

class runner
{
    using path_type = boost::filesystem::path;
    using files_iterator = std::vector<path_type>::const_iterator;

public:
    using renderer_container = std::vector<renderer_type>;

    runner(
        config const & cfg,
        std::size_t iterations,
        renderer_container const & renderers);

    result_list test(
        std::vector<std::string> const & style_names,
        report_type & report) const;

private:
    result_list test_one(
        path_type const & style_path,
        report_type & report) const;
    void parse_map_sizes(
        std::string const & str,
        std::vector<map_size> & sizes) const;

    const map_sizes_grammar<std::string::const_iterator> map_sizes_parser_;
    const config defaults_;
    const std::size_t iterations_;
    const renderer_container renderers_;
};

}

#endif
