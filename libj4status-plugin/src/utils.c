/*
 * libj4status-plugin - Library to implement a j4status plugin
 *
 * Copyright © 2012-2013 Quentin "Sardem FF7" Glidic
 *
 * This file is part of j4status.
 *
 * j4status is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * j4status is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with j4status. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <glib.h>
#include <glib/gprintf.h>

#include <nkutils-colour.h>

#include <j4status-plugin.h>

void
j4status_colour_reset(J4statusColour *colour)
{
    colour->set = FALSE;
    colour->red = 0;
    colour->green = 0;
    colour->blue = 0;
}

J4statusColour
j4status_colour_parse(const gchar *string)
{
    J4statusColour ret = { FALSE, 0, 0, 0 };
    NkColour colour_;

    if ( nk_colour_parse(string, &colour_) )
    {
        ret.set   = TRUE;
        ret.red   = colour_.red;
        ret.green = colour_.green;
        ret.blue  = colour_.blue;
    }

    return ret;
}

J4statusColour
j4status_colour_parse_length(const gchar *colour, gint length)
{
    gchar string[length + 1];
    g_sprintf(string, "%.*s", length, colour);
    return j4status_colour_parse(string);
}

const gchar *
j4status_colour_to_hex(J4statusColour colour)
{
    if ( ! colour.set )
        return NULL;

    NkColour colour_ = {
        .red   = colour.red,
        .green = colour.green,
        .blue  = colour.blue,
        .alpha = 0xff
    };

    return nk_colour_to_hex(&colour_);
}

const gchar *
j4status_colour_to_rgb(J4statusColour colour)
{
    if ( ! colour.set )
        return NULL;

    NkColour colour_ = {
        .red   = colour.red,
        .green = colour.green,
        .blue  = colour.blue,
        .alpha = 0xff
    };

    return nk_colour_to_rgba(&colour_);
}
