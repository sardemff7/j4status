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
    J4statusSection *section;
};

static gboolean
_j4status_time_update(gpointer user_data)
{
    J4statusPluginContext *context = user_data;
    GDateTime *date_time;
    date_time = g_date_time_new_now_local();

    g_free(context->section->value);
    context->section->value = g_date_time_format(date_time, context->format);
    context->section->dirty = TRUE;

    g_date_time_unref(date_time);

    context->core_interface->trigger_display(context->core);

    return TRUE;
}

static J4statusPluginContext *
_j4status_time_init(J4statusCoreContext *core, J4statusCoreInterface *core_interface)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;
    context->core_interface = core_interface;

    context->format = g_strdup("%F %T");

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
        g_key_file_free(key_file);
    }

    context->section = g_new0(J4statusSection, 1);
    context->section->name = "time";
    context->section->state = J4STATUS_STATE_NO_STATE;
    context->section->value = g_malloc0(sizeof(char) * (TIME_SIZE + 1));

    context->sections = g_list_prepend(context->sections, context->section);

    return context;
}

static void
_j4status_time_uninit(J4statusPluginContext *context)
{
    g_free(context->section->value);
    g_free(context->section);

    g_list_free(context->sections);

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
j4status_input_plugin(J4statusInputPlugin *plugin)
{
    plugin->init   = _j4status_time_init;
    plugin->uninit = _j4status_time_uninit;

    plugin->get_sections = _j4status_time_get_sections;

    plugin->start = _j4status_time_start;
    plugin->stop  = _j4status_time_stop;
}
