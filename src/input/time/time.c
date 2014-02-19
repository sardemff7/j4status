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

#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

#define TIME_SIZE 4095

struct _J4statusPluginContext {
    J4statusCoreContext *core;
    J4statusCoreInterface *core_interface;
    GList *sections;
    gchar *format;
    guint timeout_id;
};

typedef struct {
    GTimeZone *tz;
    gchar *format;
} J4statusTimeSectionContext;

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
        J4statusSection *section = section_->data;
        J4statusTimeSectionContext *section_context = j4status_section_get_user_data(section);
        zone_date_time = g_date_time_to_timezone(date_time, section_context->tz);
        j4status_section_set_value(section, g_date_time_format(zone_date_time, ( section_context->format != NULL ) ? section_context->format : context->format));
        g_date_time_unref(zone_date_time);
    }

    g_date_time_unref(date_time);

    libj4status_core_trigger_display(context->core, context->core_interface);

    return TRUE;
}

static void
_j4status_time_add_section(J4statusPluginContext *context, const gchar *timezone, gchar *format)
{
    J4statusTimeSectionContext *section_context;
    J4statusSection *section;

    section_context = g_new0(J4statusTimeSectionContext, 1);
    section_context->tz = ( timezone != NULL ) ? g_time_zone_new(timezone) : g_time_zone_new_local();
    section_context->format = format;

    section = j4status_section_new("time", section_context);
    j4status_section_set_instance(section, ( timezone != NULL ) ? timezone : "local");
    j4status_section_set_state(section, J4STATUS_STATE_NO_STATE);

    context->sections = g_list_prepend(context->sections, section);
}

static J4statusPluginContext *
_j4status_time_init(J4statusCoreContext *core, J4statusCoreInterface *core_interface)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;
    context->core_interface = core_interface;

    context->format = g_strdup("%F %T");
    gchar **timezones = NULL;
    gchar **formats = NULL;

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("Time");
    if ( key_file != NULL )
    {
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

    if ( timezones == NULL )
        _j4status_time_add_section(context, NULL, NULL);
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
                _j4status_time_add_section(context, *timezone, *format);
            }
            g_free(formats);
        }
        else
        {
            for ( timezone = timezones ; *timezone != NULL ; ++timezone )
                _j4status_time_add_section(context, *timezone, NULL);
        }
        g_strfreev(timezones);
    }
    context->sections = g_list_reverse(context->sections);

    return context;
}

static void
_j4status_time_free_section(gpointer data)
{
    J4statusSection *section = data;
    J4statusTimeSectionContext *section_context = j4status_section_get_user_data(section);

    g_free(section_context->format);
    g_time_zone_unref(section_context->tz);

    g_free(section_context);

    j4status_section_free(section);
}

static void
_j4status_time_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_time_free_section);

    g_free(context);
}

static GList **
_j4status_time_get_sections(J4statusPluginContext *context)
{
    return &context->sections;
}

static void
_j4status_time_start(J4statusPluginContext *context)
{
    _j4status_time_update(context);
    context->timeout_id = g_timeout_add_seconds(1, _j4status_time_update, context);
}

static void
_j4status_time_stop(J4statusPluginContext *context)
{
    g_source_remove(context->timeout_id);
    context->timeout_id = 0;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_time_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_time_uninit);

    libj4status_input_plugin_interface_add_get_sections_callback(interface, _j4status_time_get_sections);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_time_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_time_stop);
}
