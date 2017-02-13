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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>
#include <chrono>

#include <boost/filesystem.hpp>

namespace mapnik_render
{

struct map_size
{
    map_size(std::size_t _width, std::size_t _height) : width(_width), height(_height) { }
    map_size() { }
    std::size_t width = 0;
    std::size_t height = 0;
};

struct config
{
    config() : status(true),
               scales({ 1.0 }),
               sizes({ { 512, 512 } }),
               tiles({ { 1, 1 } }) { }

    bool status;
    std::vector<double> scales;
    std::vector<map_size> sizes;
    std::vector<map_size> tiles;
};

enum result_state : std::uint8_t
{
    STATE_OK,
    STATE_ERROR,
};

struct result
{
    std::string name;
    result_state state;
    std::string renderer_name;
    map_size size;
    map_size tiles;
    double scale_factor;
    boost::filesystem::path image_path;
    std::string error_message;
    std::chrono::high_resolution_clock::duration duration;
};

using result_list = std::vector<result>;

}

#endif
