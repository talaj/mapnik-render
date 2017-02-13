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

#include <algorithm>
#include <future>
#include <atomic>

#include <mapnik/load_map.hpp>

#include "runner.hpp"

namespace mapnik_render
{

class renderer_visitor
{
public:
    renderer_visitor(std::string const & name,
                     mapnik::Map & map,
                     map_size const & tiles,
                     double scale_factor,
                     result_list & results,
                     report_type & report,
                     std::size_t iterations)
        : name_(name),
          map_(map),
          tiles_(tiles),
          scale_factor_(scale_factor),
          results_(results),
          report_(report),
          iterations_(iterations)
    {
    }

    template <typename T, typename std::enable_if<T::renderer_type::support_tiles>::type* = nullptr>
    void operator()(T const& renderer) const
    {
        test(renderer);
    }

    template <typename T, typename std::enable_if<!T::renderer_type::support_tiles>::type* = nullptr>
    void operator()(T const & renderer) const
    {
        if (tiles_.width == 1 && tiles_.height == 1)
        {
            test(renderer);
        }
    }

private:
    template <typename T>
    void test(T const & renderer) const
    {
        map_size size { map_.width(), map_.height() };
        std::chrono::high_resolution_clock::time_point start(std::chrono::high_resolution_clock::now());
        for (std::size_t i = iterations_ ; i > 0; i--)
        {
            typename T::image_type image(render(renderer));
            if (i == 1)
            {
                std::chrono::high_resolution_clock::time_point end(std::chrono::high_resolution_clock::now());
                result r(renderer.report(image, name_, size, tiles_, scale_factor_));
                r.duration = end - start;
                mapnik::util::apply_visitor(report_visitor(r), report_);
                results_.push_back(std::move(r));
            }
        }
    }

    template <typename T, typename std::enable_if<T::renderer_type::support_tiles>::type* = nullptr>
    typename T::image_type render(T const& renderer) const
    {
        if (tiles_.width == 1 && tiles_.height == 1)
        {
            return renderer.render(map_, scale_factor_);
        }
        else
        {
            return renderer.render(map_, scale_factor_, tiles_);
        }
    }

    template <typename T, typename std::enable_if<!T::renderer_type::support_tiles>::type* = nullptr>
    typename T::image_type render(T const & renderer) const
    {
        return renderer.render(map_, scale_factor_);
    }

    std::string const & name_;
    mapnik::Map & map_;
    map_size const & tiles_;
    double scale_factor_;
    result_list & results_;
    report_type & report_;
    std::size_t iterations_;
};

runner::runner(config const & defaults,
               std::size_t iterations,
               runner::renderer_container const & renderers)
    : defaults_(defaults),
      iterations_(iterations),
      renderers_(renderers)
{
}

result_list runner::test(std::vector<std::string> const & style_names, report_type & report) const
{
    result_list results;

    for (auto const & style_name : style_names)
    {
        runner::path_type file(style_name);
        try
        {
            result_list r(test_one(file, report));
            std::move(r.begin(), r.end(), std::back_inserter(results));
        }
        catch (std::exception const& ex)
        {
            result r;
            r.state = STATE_ERROR;
            r.name = style_name;
            r.error_message = ex.what();
            r.duration = std::chrono::high_resolution_clock::duration::zero();
            results.emplace_back(r);
            mapnik::util::apply_visitor(report_visitor(r), report);
        }
    }

    return results;
}

result_list runner::test_one(runner::path_type const& style_path,
                             report_type & report) const
{
    config cfg(defaults_);
    mapnik::Map map(cfg.sizes.front().width, cfg.sizes.front().height);
    result_list results;

    mapnik::load_map(map, style_path.string(), true);

    mapnik::parameters const & params = map.get_extra_parameters();

    boost::optional<mapnik::value_integer> status = params.get<mapnik::value_integer>("status", cfg.status);

    if (!*status)
    {
        return results;
    }

    boost::optional<std::string> sizes = params.get<std::string>("sizes");

    if (sizes)
    {
        cfg.sizes.clear();
        parse_map_sizes(*sizes, cfg.sizes);
    }

    boost::optional<std::string> tiles = params.get<std::string>("tiles");

    if (tiles)
    {
        cfg.tiles.clear();
        parse_map_sizes(*tiles, cfg.tiles);
    }

    boost::optional<std::string> bbox_string = params.get<std::string>("bbox");
    mapnik::box2d<double> box;

    if (bbox_string)
    {
        box.from_string(*bbox_string);
    }

    std::string name(style_path.stem().string());

    for (auto const & size : cfg.sizes)
    {
        for (auto const & scale_factor : cfg.scales)
        {
            for (auto const & tiles_count : cfg.tiles)
            {
                if (!tiles_count.width || !tiles_count.height)
                {
                    throw std::runtime_error("Cannot render zero tiles.");
                }
                if (size.width % tiles_count.width || size.height % tiles_count.height)
                {
                    throw std::runtime_error("Tile size is not an integer.");
                }

                for (auto const & ren : renderers_)
                {
                    map.resize(size.width * scale_factor, size.height * scale_factor);
                    if (box.valid())
                    {
                        map.zoom_to_box(box);
                    }
                    else
                    {
                        map.zoom_all();
                    }
                    mapnik::util::apply_visitor(
                        renderer_visitor(name, map, tiles_count, scale_factor,
                            results, report, iterations_), ren);
                }
            }
        }
    }

    return results;
}

void runner::parse_map_sizes(std::string const & str, std::vector<map_size> & sizes) const
{
    boost::spirit::ascii::space_type space;
    std::string::const_iterator iter = str.begin();
    std::string::const_iterator end = str.end();
    if (!boost::spirit::qi::phrase_parse(iter, end, map_sizes_parser_, space, sizes))
    {
        throw std::runtime_error("Failed to parse list of sizes: '" + str + "'");
    }
}

}
