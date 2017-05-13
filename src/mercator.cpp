#ifndef MAPNIK_RENDER_MERCATOR_HPP
#define MAPNIK_RENDER_MERCATOR_HPP

#include "vector_tile_merc_tile.hpp"
#include "vector_tile_projection.hpp"

#include <sstream>

namespace mapnik_render
{

bool merc_tile::from_string(std::string const & str)
{
    std::istringstream stream(str);
    stream >> x >> y >> z;
    return stream.good();
}

mapnik::box2d<double> merc_tile::extent() const
{
    return mapnik::vector_tile_impl::merc_extent(4096, x, y, z);
}

}

#endif
