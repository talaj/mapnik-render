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

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <sstream>
#include <iomanip>
#include <fstream>
#include <memory>

#include <mapnik/map.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/util/variant.hpp>
#include <mapnik/agg_renderer.hpp>
#if defined(GRID_RENDERER)
#include <mapnik/grid/grid_renderer.hpp>
#endif

#if defined(HAVE_CAIRO)
#include <mapnik/cairo/cairo_renderer.hpp>
#include <mapnik/cairo/cairo_image_util.hpp>
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#endif

#if defined(SVG_RENDERER)
#include <mapnik/svg/output/svg_renderer.hpp>
#endif

#include <vector_tile_merc_tile.hpp>
#include <vector_tile_processor.hpp>
#include <vector_tile_compression.hpp>

#include <boost/filesystem.hpp>

namespace mapnik_render
{

template <typename ImageType>
struct raster_renderer_base
{
    using image_type = ImageType;

    static constexpr const char * ext = ".png";
    static constexpr const bool support_tiles = true;

    void save(image_type const & image, boost::filesystem::path const& path) const
    {
        mapnik::save_to_file(image, path.string(), "png32");
    }
};

struct vector_renderer_base
{
    using image_type = std::string;

    static constexpr const bool support_tiles = false;

    void save(image_type const & image, boost::filesystem::path const& path) const
    {
        std::ofstream file(path.string().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Cannot open file for writing: " + path.string());
        }
        file << image;
    }
};

struct agg_renderer : raster_renderer_base<mapnik::image_rgba8>
{
    static constexpr const char * name = "agg";

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        image_type image(map.width(), map.height());
        mapnik::agg_renderer<image_type> ren(map, image, scale_factor);
        ren.apply();
        return image;
    }
};

#if defined(HAVE_CAIRO)
struct cairo_renderer : raster_renderer_base<mapnik::image_rgba8>
{
    static constexpr const char * name = "cairo";

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        mapnik::cairo_surface_ptr image_surface(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, map.width(), map.height()),
            mapnik::cairo_surface_closer());
        mapnik::cairo_ptr image_context(mapnik::create_context(image_surface));
        mapnik::cairo_renderer<mapnik::cairo_ptr> ren(map, image_context, scale_factor);
        ren.apply();
        image_type image(map.width(), map.height());
        mapnik::cairo_image_to_rgba8(image, image_surface);
        return image;
    }
};

using surface_create_type = cairo_surface_t *(&)(cairo_write_func_t, void *, double, double);

template <surface_create_type SurfaceCreateFunction>
struct cairo_vector_renderer : vector_renderer_base
{
    static cairo_status_t write(void *closure,
                                const unsigned char *data,
                                unsigned int length)
    {
        std::ostringstream & ss = *reinterpret_cast<std::ostringstream*>(closure);
        ss.write(reinterpret_cast<char const *>(data), length);
        return ss ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
    }

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        std::ostringstream ss(std::stringstream::binary);
        mapnik::cairo_surface_ptr image_surface(
            SurfaceCreateFunction(write, &ss, map.width(), map.height()),
            mapnik::cairo_surface_closer());
        mapnik::cairo_ptr image_context(mapnik::create_context(image_surface));
        mapnik::cairo_renderer<mapnik::cairo_ptr> ren(map, image_context, scale_factor);
        ren.apply();
        cairo_surface_finish(&*image_surface);
        return ss.str();
    }
};

#ifdef CAIRO_HAS_SVG_SURFACE
struct cairo_svg_renderer : cairo_vector_renderer<cairo_svg_surface_create_for_stream>
{
    static constexpr const char * name = "cairo-svg";
    static constexpr const char * ext = ".svg";
};
#endif

#ifdef CAIRO_HAS_PS_SURFACE
struct cairo_ps_renderer : cairo_vector_renderer<cairo_ps_surface_create_for_stream>
{
    static constexpr const char * name = "cairo-ps";
    static constexpr const char * ext = ".ps";
};
#endif

#ifdef CAIRO_HAS_PDF_SURFACE
struct cairo_pdf_renderer : cairo_vector_renderer<cairo_pdf_surface_create_for_stream>
{
    static constexpr const char * name = "cairo-pdf";
    static constexpr const char * ext = ".pdf";
};
#endif
#endif

#if defined(SVG_RENDERER)
struct svg_renderer : vector_renderer_base
{
    static constexpr const char * name = "svg";
    static constexpr const char * ext = ".svg";

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        std::stringstream ss;
        std::ostream_iterator<char> output_stream_iterator(ss);
        mapnik::svg_renderer<std::ostream_iterator<char>> ren(map, output_stream_iterator, scale_factor);
        ren.apply();
        return ss.str();
    }
};
#endif

#if defined(GRID_RENDERER)
struct grid_renderer : raster_renderer_base<mapnik::image_rgba8>
{
    static constexpr const char * name = "grid";

    void convert(mapnik::grid::data_type const & grid, image_type & image) const
    {
        for (std::size_t y = 0; y < grid.height(); ++y)
        {
            mapnik::grid::value_type const * grid_row = grid.get_row(y);
            image_type::pixel_type * image_row = image.get_row(y);
            for (std::size_t x = 0; x < grid.width(); ++x)
            {
                mapnik::grid::value_type val = grid_row[x];

                if (val == mapnik::grid::base_mask)
                {
                    image_row[x] = 0;
                    continue;
                }
                if (val < 0)
                {
                    throw std::runtime_error("grid renderer: feature id is negative.");
                }

                val *= 100;

                if (val > 0x00ffffff)
                {
                    throw std::runtime_error("grid renderer: feature id is too high.");
                }

                image_row[x] = val | 0xff000000;
            }
        }
    }

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        mapnik::grid grid(map.width(), map.height(), "__id__");
        mapnik::grid_renderer<mapnik::grid> ren(map, grid, scale_factor);
        ren.apply();
        image_type image(map.width(), map.height());
        convert(grid.data(), image);
        return image;
    }
};
#endif

struct mvt_renderer : vector_renderer_base
{
    static constexpr const char * name = "mvt";
    static constexpr const char * ext = ".mvt";

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        std::uint64_t x = 2257;
        std::uint64_t y = 1393;
        std::uint64_t z = 12;
        std::uint32_t tile_size = 4096;
        std::int32_t buffer_size = 0;
        double scale_denom = 0.0;
        int offset_x = 0;
        int offset_y = 0;

        mapnik::vector_tile_impl::processor proc(map);
        mapnik::vector_tile_impl::merc_tile tile(proc.create_tile(
            x, y, z, tile_size, buffer_size, scale_denom, offset_x, offset_y));
        std::string output;
        mapnik::vector_tile_impl::zlib_compress(tile.data(), tile.size(), output);
        return output;
    }
};

template <typename T>
void set_rectangle(T const & src, T & dst, std::size_t x, std::size_t y)
{
    mapnik::box2d<int> ext0(0, 0, dst.width(), dst.height());
    mapnik::box2d<int> ext1(x, y, x + src.width(), y + src.height());

    if (ext0.intersects(ext1))
    {
        mapnik::box2d<int> box = ext0.intersect(ext1);
        for (std::size_t pix_y = box.miny(); pix_y < static_cast<std::size_t>(box.maxy()); ++pix_y)
        {
            typename T::pixel_type * row_to =  dst.get_row(pix_y);
            typename T::pixel_type const * row_from = src.get_row(pix_y - y);

            for (std::size_t pix_x = box.minx(); pix_x < static_cast<std::size_t>(box.maxx()); ++pix_x)
            {
                row_to[pix_x] = row_from[pix_x - x];
            }
        }
    }
}

template <typename Renderer>
class renderer
{
public:
    using renderer_type = Renderer;
    using image_type = typename Renderer::image_type;

    renderer(boost::filesystem::path const & _output_dir)
        : ren(), output_dir(_output_dir)
    {
    }

    image_type render(mapnik::Map const & map, double scale_factor) const
    {
        return ren.render(map, scale_factor);
    }

    image_type render(mapnik::Map & map, double scale_factor, map_size const & tiles) const
    {
        mapnik::box2d<double> box = map.get_current_extent();
        image_type image(map.width(), map.height());
        map.resize(image.width() / tiles.width, image.height() / tiles.height);
        double tile_box_width = box.width() / tiles.width;
        double tile_box_height = box.height() / tiles.height;
        for (std::size_t tile_y = 0; tile_y < tiles.height; tile_y++)
        {
            for (std::size_t tile_x = 0; tile_x < tiles.width; tile_x++)
            {
                mapnik::box2d<double> tile_box(
                    box.minx() + tile_x * tile_box_width,
                    box.miny() + tile_y * tile_box_height,
                    box.minx() + (tile_x + 1) * tile_box_width,
                    box.miny() + (tile_y + 1) * tile_box_height);
                map.zoom_to_box(tile_box);
                image_type tile(ren.render(map, scale_factor));
                set_rectangle(tile, image, tile_x * tile.width(), (tiles.height - 1 - tile_y) * tile.height());
            }
        }
        return image;
    }

    result report(image_type const & image,
                  std::string const & name,
                  map_size const & size,
                  map_size const & tiles,
                  double scale_factor) const
    {
        result res;

        res.state = STATE_OK;
        res.name = name;
        res.renderer_name = Renderer::name;
        res.scale_factor = scale_factor;
        res.size = size;
        res.tiles = tiles;

        boost::filesystem::create_directories(output_dir);
        boost::filesystem::path path = output_dir / image_file_name(name, size, tiles, scale_factor);
        res.image_path = path;
        ren.save(image, path);

        return res;
    }

private:
    std::string image_file_name(std::string const & test_name,
                                map_size const & size,
                                map_size const & tiles,
                                double scale_factor) const
    {
        std::stringstream s;
        s << test_name << '-' << (size.width / scale_factor) << '-' << (size.height / scale_factor) << '-';
        if (tiles.width > 1 || tiles.height > 1)
        {
            s << tiles.width << 'x' << tiles.height << '-';
        }
        s << std::fixed << std::setprecision(1) << scale_factor << '-' << Renderer::name;
        s << Renderer::ext;
        return s.str();
    }

    const Renderer ren;
    const boost::filesystem::path output_dir;
};

using renderer_type = mapnik::util::variant<renderer<agg_renderer>
#if defined(HAVE_CAIRO)
                                            ,renderer<cairo_renderer>
#ifdef CAIRO_HAS_SVG_SURFACE
                                            ,renderer<cairo_svg_renderer>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
                                            ,renderer<cairo_ps_renderer>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
                                            ,renderer<cairo_pdf_renderer>
#endif
#endif
#if defined(SVG_RENDERER)
                                            ,renderer<svg_renderer>
#endif
#if defined(GRID_RENDERER)
                                            ,renderer<grid_renderer>
#endif
                                            ,renderer<mvt_renderer>
                                            >;

}

#endif
