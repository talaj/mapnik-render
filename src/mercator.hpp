#ifndef MAPNIK_RENDER_MERCATOR_HPP
#define MAPNIK_RENDER_MERCATOR_HPP

#include <mapnik/geometry/box2d.hpp>

namespace mapnik_render
{

struct merc_tile
{
    unsigned long x, y, z;

    bool from_string(std::string const & s);
    mapnik::box2d<double> extent() const;
};

}

#endif

