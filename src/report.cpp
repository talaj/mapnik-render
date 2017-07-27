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

#include <iomanip>
#include <fstream>
#include <numeric>
#include <map>

#include "report.hpp"

namespace mapnik_render
{

void console_report::report(result const & r)
{
    s << '"' << r.name << '-' << r.size.width << '-' << r.size.height;
    if (r.tiles.width > 1 || r.tiles.height > 1)
    {
        s << '-' << r.tiles.width << 'x' << r.tiles.height;
    }
    s << '-' << std::fixed << std::setprecision(1) << r.scale_factor << "\" with " << r.renderer_name << "... ";

    switch (r.state)
    {
        case STATE_OK:
            s << "OK";
            break;
        case STATE_ERROR:
            s << "ERROR (" << r.error_message << ")";
            break;
    }

    if (show_duration)
    {
        s << " (" << std::chrono::duration_cast<std::chrono::milliseconds>(r.duration).count() << " milliseconds)";
    }

    s << std::endl;
}

unsigned console_report::summary(result_list const & results)
{
    unsigned ok = 0;
    unsigned error = 0;

    using namespace std::chrono;
    using duration_map_type = std::map<std::string, high_resolution_clock::duration>;
    duration_map_type durations;

    for (auto const & r : results)
    {
        switch (r.state)
        {
            case STATE_OK: ok++; break;
            case STATE_ERROR: error++; break;
        }

        if (show_duration)
        {
            duration_map_type::iterator duration = durations.find(r.renderer_name);
            if (duration == durations.end())
            {
                durations.emplace(r.renderer_name, r.duration);
            }
            else
            {
                duration->second += r.duration;
            }
        }
    }

    s << std::endl;
    s << "Rendering: " << ok << " ok / " << error << " errors" << std::endl;

    if (show_duration)
    {
        high_resolution_clock::duration total(0);
        for (auto const & duration : durations)
        {
            s << duration.first << ": \t" << duration_cast<milliseconds>(duration.second).count()
              << " milliseconds" << std::endl;
            total += duration.second;
        }
        s << "total: \t" << duration_cast<milliseconds>(total).count() << " milliseconds" << std::endl;
    }

    return error;
}

void console_short_report::report(result const & r)
{
    switch (r.state)
    {
        case STATE_OK:
            //s << ".";
            break;
        case STATE_ERROR:
            s << "ERROR (" << r.error_message << ")\n";
            break;
    }
}

}
