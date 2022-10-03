/*
 * libj4status-plugin - Library to implement a j4status plugin
 *
 * Copyright © 2012-2018 Quentin "Sardem FF7" Glidic
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

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "nkutils-format-string.h"
#include "nkutils-colour.h"

#include "j4status-plugin.h"

J4STATUS_EXPORT J4statusFormatString *
j4status_format_string_parse(gchar *string, const gchar * const *tokens, guint64 size, const gchar *default_string, guint64 *used_tokens)
{
    NkFormatString *token_list = NULL;

    if ( string != NULL )
        token_list = nk_format_string_parse_enum(string, '$', tokens, size, used_tokens, NULL);

    if ( token_list == NULL )
        token_list = nk_format_string_parse_enum(g_strdup(default_string), '$', tokens, size, used_tokens, NULL);

    return token_list;
}

J4STATUS_EXPORT J4statusFormatString *
j4status_format_string_ref(J4statusFormatString *format_string)
{
    if ( format_string == NULL )
        return NULL;

    return nk_format_string_ref(format_string);
}

J4STATUS_EXPORT void
j4status_format_string_unref(J4statusFormatString *format_string)
{
    if ( format_string == NULL )
        return;

    nk_format_string_unref(format_string);
}

J4STATUS_EXPORT gchar *
j4status_format_string_replace(const J4statusFormatString *format_string, J4statusFormatStringReplaceCallback callback, gconstpointer user_data)
{
    if ( format_string == NULL )
        return NULL;

    return nk_format_string_replace(format_string, (NkFormatStringReplaceReferenceCallback) callback, (gpointer) user_data);
}

J4STATUS_EXPORT void
j4status_colour_reset(J4statusColour *colour)
{
    colour->set = FALSE;
    colour->red = 0;
    colour->green = 0;
    colour->blue = 0;
    colour->alpha = 0xff;
}

J4STATUS_EXPORT J4statusColour
j4status_colour_parse(const gchar *string)
{
    J4statusColour ret = { FALSE, 0, 0, 0, 0xff };
    NkColour colour_;

    if ( nk_colour_parse(string, &colour_) )
    {
        ret.set   = TRUE;
        ret.red   = colour_.red    * 255;
        ret.green = colour_.green  * 255;
        ret.blue  = colour_.blue   * 255;
        ret.alpha  = colour_.alpha * 255;
    }

    return ret;
}

J4STATUS_EXPORT J4statusColour
j4status_colour_parse_length(const gchar *colour, gint length)
{
    gchar string[length + 1];
    g_sprintf(string, "%.*s", length, colour);
    return j4status_colour_parse(string);
}

J4STATUS_EXPORT const gchar *
j4status_colour_to_hex(J4statusColour colour)
{
    if ( ! colour.set )
        return NULL;

    NkColour colour_ = {
        .red   = colour.red   / 255.,
        .green = colour.green / 255.,
        .blue  = colour.blue  / 255.,
        .alpha = colour.alpha / 255.,
    };

    return nk_colour_to_hex(&colour_);
}

J4STATUS_EXPORT const gchar *
j4status_colour_to_rgb(J4statusColour colour)
{
    if ( ! colour.set )
        return NULL;

    NkColour colour_ = {
        .red   = colour.red   / 255.,
        .green = colour.green / 255.,
        .blue  = colour.blue  / 255.,
        .alpha = colour.alpha / 255.,
    };

    return nk_colour_to_rgba(&colour_);
}
