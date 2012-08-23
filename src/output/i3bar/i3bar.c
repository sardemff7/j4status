/*
 * j4status - Status line generator
 *
 * Copyright © 2012 Quentin "Sardem FF7" Glidic
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

#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>
#include <yajl/yajl_gen.h>

#include <j4status-plugin.h>

#include <libj4status-config.h>

struct _J4statusPluginContext {
    struct {
        gchar *no_state;
        gchar *unavailable;
        gchar *bad;
        gchar *average;
        gchar *good;
    } colours;
    yajl_gen json_gen;
};

static void
_j4status_i3bar_update_colour(gchar **colour, GKeyFile *key_file, gchar *name)
{
    gchar *config;
    config = g_key_file_get_string(key_file, "i3bar", name, NULL);
    if ( config == NULL )
        return;

    if ( ( config[0] == '#' ) && ( strlen(config) == strlen("#000000") ) )
    {
        gchar *end;
        g_ascii_strtoull(config+1, &end, 16);
        if ( end == (config + strlen("#000000")) )
        {
            *colour = config;
            config = NULL;
        }
    }
    else if ( ( config[0] == '\0' ) || ( g_str_equal(config, "none") ) )
    {
        g_free(*colour);
        *colour = NULL;
    }

    g_free(config);
}

static J4statusPluginContext *
_j4status_i3bar_init(J4statusCoreContext *core, J4statusCoreInterface *core_interface)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);

    context->colours.no_state    = NULL;
    context->colours.unavailable = g_strdup("#0000FF");
    context->colours.bad         = g_strdup("#FF0000");
    context->colours.average     = g_strdup("#FFFF00");
    context->colours.good        = g_strdup("#00FF00");

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("i3bar");
    if ( key_file != NULL )
    {
        _j4status_i3bar_update_colour(&context->colours.no_state, key_file, "NoStateColour");
        _j4status_i3bar_update_colour(&context->colours.unavailable, key_file, "UnavailableColour");
        _j4status_i3bar_update_colour(&context->colours.bad, key_file, "BadColour");
        _j4status_i3bar_update_colour(&context->colours.average, key_file, "AverageColour");
        _j4status_i3bar_update_colour(&context->colours.good, key_file, "GoodColour");
        g_key_file_free(key_file);
    }

    yajl_gen json_gen;
    json_gen = yajl_gen_alloc(NULL);

    yajl_gen_map_open(json_gen);
    yajl_gen_string(json_gen, (const unsigned char *)"version", strlen("version"));
    yajl_gen_integer(json_gen, 1);
    yajl_gen_string(json_gen, (const unsigned char *)"stop_with_usr1", strlen("stop_with_usr1"));
    yajl_gen_bool(json_gen, TRUE);
    yajl_gen_map_close(json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_free(json_gen);

    context->json_gen = yajl_gen_alloc(NULL);
    yajl_gen_array_open(context->json_gen);
    yajl_gen_get_buf(json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_clear(context->json_gen);


    return context;
}

static void
_j4status_i3bar_uninit(J4statusPluginContext *context)
{
    yajl_gen_free(context->json_gen);

    g_free(context);
}

static void
_j4status_i3bar_print(J4statusPluginContext *context, GList *sections_)
{
    yajl_gen_array_open(context->json_gen);
    J4statusSection *section;
    for ( ; sections_ != NULL ; sections_ = g_list_next(sections_) )
    {
        GList **section__ = sections_->data;
        GList *section_;
        for ( section_ = *section__ ; section_ != NULL ; section_ = g_list_next(section_) )
        {
            section = section_->data;
            if ( section->dirty )
            {
                g_free(section->line_cache);
                section->dirty = FALSE;
                if ( section->value == NULL )
                {
                    section->line_cache = NULL;
                    continue;
                }
                if ( section->label != NULL )
                    section->line_cache = g_strdup_printf("%s: %s", section->label, section->value);
                else
                    section->line_cache = g_strdup(section->value);
            }
            if ( section->line_cache == NULL )
                continue;

            yajl_gen_map_open(context->json_gen);

            if ( section->name != NULL )
            {
                yajl_gen_string(context->json_gen, (const unsigned char *)"name", strlen("name"));
                yajl_gen_string(context->json_gen, (const unsigned char *)section->name, strlen(section->name));
            }

            if ( section->instance != NULL )
            {
                yajl_gen_string(context->json_gen, (const unsigned char *)"instance", strlen("instance"));
                yajl_gen_string(context->json_gen, (const unsigned char *)section->instance, strlen(section->instance));
            }

            const gchar *colour = NULL;
            switch ( section->state )
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
            case J4STATUS_STATE_URGENT:
                colour = context->colours.bad;
                yajl_gen_string(context->json_gen, (const unsigned char *)"urgent", strlen("urgent"));
                yajl_gen_bool(context->json_gen, TRUE);
            break;
            }
            if ( colour != NULL )
            {
                yajl_gen_string(context->json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(context->json_gen, (const unsigned char *)colour, strlen("#000000"));
            }

            yajl_gen_string(context->json_gen, (const unsigned char *)"full_text", strlen("full_text"));
            yajl_gen_string(context->json_gen, (const unsigned char *)section->line_cache, strlen(section->line_cache));

            yajl_gen_map_close(context->json_gen);
        }
    }
    yajl_gen_array_close(context->json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(context->json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_clear(context->json_gen);
}

void
j4status_output_plugin(J4statusOutputPlugin *plugin)
{
    plugin->init   = _j4status_i3bar_init;
    plugin->uninit = _j4status_i3bar_uninit;

    plugin->print = _j4status_i3bar_print;
}
