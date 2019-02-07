/*
 * j4status - Status line generator
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

#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-input.h>

#define TIME_SIZE 4095

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    guint64 interval;
    gchar *format;
    GList *sections;
    guint timeout_id;
};

typedef struct {
    J4statusSection *section;
    GTimeZone *tz;
    gchar *format;
} J4statusTimeSection;

static gboolean
_j4status_time_update(gpointer user_data)
{
    J4statusPluginContext *context = user_data;
    GDateTime *date_time;
    GDateTime *zone_date_time;
    date_time = g_date_time_new_now_utc();

    GList *section_;
    for ( section_ = context->sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        J4statusTimeSection *section = section_->data;
        zone_date_time = g_date_time_to_timezone(date_time, section->tz);
        j4status_section_set_value(section->section, g_date_time_format(zone_date_time, ( section->format != NULL ) ? section->format : context->format));
        g_date_time_unref(zone_date_time);
    }

    g_date_time_unref(date_time);

    return TRUE;
}

static void
_j4status_time_section_free(gpointer data)
{
    J4statusTimeSection *section = data;

    g_free(section->format);
    g_time_zone_unref(section->tz);


    j4status_section_free(section->section);

    g_free(section);
}

static void
_j4status_time_section_new(J4statusPluginContext *context, const gchar *timezone, gchar *format)
{
    J4statusTimeSection *section;

    section = g_new0(J4statusTimeSection, 1);
    section->tz = ( timezone != NULL ) ? g_time_zone_new(timezone) : g_time_zone_new_local();
    section->format = format;
    section->section = j4status_section_new(context->core);

    timezone = ( timezone != NULL ) ? timezone : "local";

    j4status_section_set_name(section->section, "time");
    j4status_section_set_instance(section->section, timezone);

    if ( j4status_section_insert(section->section) )
        context->sections = g_list_prepend(context->sections, section);
    else
        _j4status_time_section_free(section);
}

static void _j4status_time_uninit(J4statusPluginContext *context);

static J4statusPluginContext *
_j4status_time_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->format = g_strdup("%F %T");
    gchar **timezones = NULL;
    gchar **formats = NULL;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("Time");
    if ( key_file != NULL )
    {
        context->interval = g_key_file_get_uint64(key_file, "Time", "Interval", NULL);

        gchar *format;
        format = g_key_file_get_string(key_file, "Time", "Format", NULL);
        if ( format != NULL )
        {
            g_free(context->format);
            context->format = format;
        }

        gsize timezones_length;
        gsize formats_length;

        timezones = g_key_file_get_string_list(key_file, "Time", "Zones", &timezones_length, NULL);
        formats = g_key_file_get_string_list(key_file, "Time", "Formats", &formats_length, NULL);
        if ( ( formats != NULL ) && ( formats_length != timezones_length ) )
        {
            g_warning("You should specify one format per timezone. Will use global format");
            g_strfreev(formats);
            formats = NULL;
        }

        g_key_file_free(key_file);
    }
    if ( context->interval < 1 )
        context->interval = 1;

    if ( timezones == NULL )
        _j4status_time_section_new(context, NULL, NULL);
    else
    {
        gchar **timezone;
        if ( formats != NULL )
        {
            gchar **format;
            for ( timezone = timezones, format = formats ; *timezone != NULL ; ++timezone, ++format )
            {
                if ( **format == '\0' )
                {
                    g_free(*format);
                    *format = NULL;
                }
                _j4status_time_section_new(context, *timezone, *format);
            }
            g_free(formats);
        }
        else
        {
            for ( timezone = timezones ; *timezone != NULL ; ++timezone )
                _j4status_time_section_new(context, *timezone, NULL);
        }
        g_strfreev(timezones);
    }

    if ( context->sections == NULL )
    {
        _j4status_time_uninit(context);
        return NULL;
    }

    return context;
}

static void
_j4status_time_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_time_section_free);

    g_free(context->format);

    g_free(context);
}

static void
_j4status_time_start(J4statusPluginContext *context)
{
    _j4status_time_update(context);
    context->timeout_id = g_timeout_add_seconds(context->interval, _j4status_time_update, context);
}

static void
_j4status_time_stop(J4statusPluginContext *context)
{
    g_source_remove(context->timeout_id);
    context->timeout_id = 0;
}

J4STATUS_EXPORT void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_time_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_time_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_time_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_time_stop);
}
