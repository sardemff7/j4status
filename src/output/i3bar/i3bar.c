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
_j4status_i3bar_init(J4statusCoreInterface *core)
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
    yajl_gen_string(json_gen, (const unsigned char *)"stop_signal", strlen("stop_signal"));
    yajl_gen_integer(json_gen, SIGHUP);
    yajl_gen_string(json_gen, (const unsigned char *)"cont_signal", strlen("cont_signal"));
    yajl_gen_integer(json_gen, SIGHUP);
    yajl_gen_map_close(json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_free(json_gen);

    context->json_gen = yajl_gen_alloc(NULL);
    yajl_gen_array_open(context->json_gen);
    yajl_gen_get_buf(context->json_gen, &buffer, &length);
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
_j4status_i3bar_print(J4statusPluginContext *context, GList *sections)
{
    yajl_gen_array_open(context->json_gen);
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;
        const gchar *cache;
        if ( j4status_section_is_dirty(section) )
        {
            gchar *new_cache = NULL;
            const gchar *label;
            const gchar *value;

            value = j4status_section_get_value(section);
            if ( value != NULL )
            {
                label = j4status_section_get_label(section);
                if ( label != NULL )
                    new_cache = g_strdup_printf("%s: %s", label, value);
                else
                    new_cache = g_strdup(value);
            }
            j4status_section_set_cache(section, new_cache);
            cache = new_cache;
        }
        else
            cache = j4status_section_get_cache(section);
        if ( cache == NULL )
            continue;

        yajl_gen_map_open(context->json_gen);

        const gchar *name;
        name = j4status_section_get_name(section);
        if ( name != NULL )
        {
            yajl_gen_string(context->json_gen, (const unsigned char *)"name", strlen("name"));
            yajl_gen_string(context->json_gen, (const unsigned char *)name, strlen(name));
        }

        const gchar *instance;
        instance = j4status_section_get_instance(section);
        if ( instance != NULL )
        {
            yajl_gen_string(context->json_gen, (const unsigned char *)"instance", strlen("instance"));
            yajl_gen_string(context->json_gen, (const unsigned char *)instance, strlen(instance));
        }

        const gchar *colour = NULL;
        switch ( j4status_section_get_state(section) )
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
        yajl_gen_string(context->json_gen, (const unsigned char *)cache, strlen(cache));

        yajl_gen_map_close(context->json_gen);
    }
    yajl_gen_array_close(context->json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(context->json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_clear(context->json_gen);
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_init_callback(interface, _j4status_i3bar_init);
    libj4status_output_plugin_interface_add_uninit_callback(interface, _j4status_i3bar_uninit);

    libj4status_output_plugin_interface_add_print_callback(interface, _j4status_i3bar_print);
}
