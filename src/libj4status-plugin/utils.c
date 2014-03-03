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

#include <j4status-plugin.h>

static gchar _j4status_colour_hex[8];
static gchar _j4status_colour_rgb[17];

void
j4status_colour_reset(J4statusColour *colour)
{
    colour->set = FALSE;
    colour->red = 0;
    colour->green = 0;
    colour->blue = 0;
}

static gboolean
_j4status_colour_parse_internal(const gchar *sr, const gchar *sg, const gchar *sb, guint8 base, J4statusColour *colour)
{
    gchar *next;

    colour->red = g_ascii_strtoull(sr, &next, base);
    if ( sr == next ) return FALSE;
    colour->green = g_ascii_strtoull(sg, &next, base);
    if ( sg == next ) return FALSE;
    colour->blue = g_ascii_strtoull(sb, &next, base);
    if ( sb == next ) return FALSE;

    return TRUE;
}

J4statusColour
j4status_colour_parse(const gchar *colour)
{
    J4statusColour ret = { FALSE, 0, 0, 0 };

    if ( g_str_has_prefix(colour, "#") )
    {
        gchar hr[3] = {0};
        gchar hg[3] = {0};
        gchar hb[3] = {0};

        colour += strlen("#");
        switch ( strlen(colour) )
        {
        case 6: /* rrggbb */
            hr[0] = colour[0]; hr[1] = colour[1];
            hg[0] = colour[2]; hg[1] = colour[3];
            hb[0] = colour[4]; hb[1] = colour[5];
        break;
        case 3: /* rgb */
            hr[0] = hr[1] = colour[0];
            hg[0] = hg[1] = colour[1];
            hb[0] = hb[1] = colour[2];
        break;
        }
        ret.set = _j4status_colour_parse_internal(hr, hg, hb, 16, &ret);
    }
    else if ( g_str_has_prefix(colour, "rgb(") && g_str_has_suffix(colour, ")") )
    {
        colour += strlen("rgb(");
        const gchar *rr, *rg, *rb;
        rr = colour;

        colour = strchr(colour, ',');
        do ++colour; while ( *colour == ' ' );

        rg = colour;

        colour = strchr(colour, ',');
        do ++colour; while ( *colour == ' ' );

        rb = colour;

        ret.set = _j4status_colour_parse_internal(rr, rg, rb, 10, &ret);
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

    g_sprintf(_j4status_colour_hex, "#%02x%02x%02x", colour.red, colour.green, colour.blue);

    return _j4status_colour_hex;
}

const gchar *
j4status_colour_to_rgb(J4statusColour colour)
{
    if ( ! colour.set )
        return NULL;

    g_sprintf(_j4status_colour_rgb, "rgb(%u,%u,%u)", colour.red, colour.green, colour.blue);

    return _j4status_colour_rgb;
}
