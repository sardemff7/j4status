/*
 * j4status - Status line generator
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-output.h>

#include <libj4status-config.h>

#define COLOUR_STR(n) gchar n[16] = {0}; /* strlen("\e[38;5;xxxm\e[5m\x07") */
#define MAX_LINE 256

struct _J4statusPluginContext {
    struct {
        J4statusColour no_state;
        J4statusColour unavailable;
        J4statusColour bad;
        J4statusColour average;
        J4statusColour good;
    } colours;
    gboolean align;
};

static void
_j4status_flat_set_colour(gchar out[], J4statusColour colour, gboolean important)
{
    gsize o = 0;
    if ( colour.set )
        o = g_sprintf(out, "\e[38;5;%03hum", 16 + ( ( colour.red / 51 ) * 36 ) + ( ( colour.green / 51 ) * 6 ) + ( colour.blue / 51 ));
    if ( important )
        g_sprintf(out + o, "\e[5m\x07");
}

static void
_j4status_flat_print(J4statusPluginContext *context, GList *sections)
{
    GList *section_;
    J4statusSection *section;
    gboolean first = TRUE;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;
        const gchar *cache;
        if ( j4status_section_is_dirty(section) )
        {
            gchar *new_cache = NULL;
            const gchar *value;
            value = j4status_section_get_value(section);
            if ( value != NULL )
            {
                J4statusColour colour = {0};
                COLOUR_STR(colour_str);


                J4statusState state = j4status_section_get_state(section);
                gboolean urgent = ( state & J4STATUS_STATE_URGENT );
                colour = j4status_section_get_colour(section);
                if ( ! colour.set )
                switch ( state & ~J4STATUS_STATE_FLAGS )
                {
                case J4STATUS_STATE_NO_STATE:
                    colour = context->colours.no_state;
                break;
                case J4STATUS_STATE_UNAVAILABLE:
                    colour = context->colours.unavailable;
                break;
                case J4STATUS_STATE_BAD:
                    colour = context->colours.bad;
                break;
                case J4STATUS_STATE_AVERAGE:
                    colour = context->colours.average;
                break;
                case J4STATUS_STATE_GOOD:
                    colour = context->colours.good;
                break;
                }
                _j4status_flat_set_colour(colour_str, colour, urgent);

                gint64 max_width;
                max_width = j4status_section_get_max_width(section);

                gsize s = 1, l = 0, r = 0;
                if ( max_width < 0 )
                {
                    s = -max_width;
                    switch ( j4status_section_get_align(section) )
                    {
                    case J4STATUS_ALIGN_CENTER:
                        l = s / 2;
                        r = ( s + 1 ) / 2;
                    break;
                    case J4STATUS_ALIGN_LEFT:
                        r = s;
                    break;
                    case J4STATUS_ALIGN_RIGHT:
                        l = s;
                    break;
                    }
                }
                gchar align_left[s], align_right[s];
                memset(align_left, ' ', l); align_left[l] = '\0';
                memset(align_right, ' ', r); align_right[r] = '\0';

                const gchar *label;
                label = j4status_section_get_label(section);
                if ( label != NULL )
                {
                    COLOUR_STR(label_colour_str);
                    _j4status_flat_set_colour(label_colour_str, j4status_section_get_label_colour(section), FALSE);

                    new_cache = g_strdup_printf("\e[0m%s%s\e[0m: %s%s%s\e[0m%s", label_colour_str, label, align_left, colour_str, value, align_right);
                }
                else
                    new_cache = g_strdup_printf("%s\e[0m%s%s\e[0m%s", align_left, colour_str, value, align_right);
            }
            j4status_section_set_cache(section, new_cache);
            cache = new_cache;
        }
        else
            cache = j4status_section_get_cache(section);
        if ( cache == NULL )
            continue;
        if ( first )
            first = FALSE;
        else
            g_printf(" | ");
        g_printf("%s", cache);
    }
    g_printf("\n");
}

static void
_j4status_flat_update_colour(J4statusColour *colour, GKeyFile *key_file, gchar *name)
{
    gchar *config;
    config = g_key_file_get_string(key_file, "Flat", name, NULL);
    if ( config == NULL )
        return;

    *colour = j4status_colour_parse(config);
    g_free(config);
}

static J4statusPluginContext *
_j4status_flat_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("Flat");
    if ( key_file != NULL )
        context->align = g_key_file_get_boolean(key_file, "Flat", "Align", NULL);

    if (
#ifdef G_OS_UNIX
        isatty(1) &&
#endif /* G_OS_UNIX */
        ( g_str_has_suffix(g_getenv("TERM"), "-256color") || g_str_has_suffix(g_getenv("TERM"), "-256colour") )
        )
    {
        context->colours.unavailable.set = TRUE;
        context->colours.unavailable.blue = 0xff;
        context->colours.bad.set = TRUE;
        context->colours.bad.red = 0xff;
        context->colours.average.set = TRUE;
        context->colours.average.green = 0xff;
        context->colours.average.blue = 0xff;
        context->colours.good.set = TRUE;
        context->colours.good.green = 0xff;

        if ( key_file != NULL )
        {
            _j4status_flat_update_colour(&context->colours.no_state, key_file, "NoStateColour");
            _j4status_flat_update_colour(&context->colours.unavailable, key_file, "UnavailableColour");
            _j4status_flat_update_colour(&context->colours.bad, key_file, "BadColour");
            _j4status_flat_update_colour(&context->colours.average, key_file, "AverageColour");
            _j4status_flat_update_colour(&context->colours.good, key_file, "GoodColour");
        }
    }

    if ( key_file != NULL )
    g_key_file_free(key_file);

    return context;
}

static void
_j4status_flat_uninit(J4statusPluginContext *context)
{
    g_free(context);
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_init_callback(interface, _j4status_flat_init);
    libj4status_output_plugin_interface_add_uninit_callback(interface, _j4status_flat_uninit);

    libj4status_output_plugin_interface_add_print_callback(interface, _j4status_flat_print);
}
