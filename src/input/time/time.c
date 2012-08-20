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

static struct {
    GList *sections;
    gchar *format;
    J4statusSection *section;
} context;

static gboolean
_j4status_time_update(gpointer user_data)
{
    GDateTime *date_time;
    date_time = g_date_time_new_now_local();

    g_free(context.section->value);
    context.section->value = g_date_time_format(date_time, context.format);
    context.section->dirty = TRUE;

    g_date_time_unref(date_time);

    return TRUE;
}

GList **
j4status_input()
{
    context.sections = NULL;
    context.format = g_strdup("%F %T");

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("Time");
    if ( key_file != NULL )
    {
        gchar *format;
        format = g_key_file_get_string(key_file, "Time", "Format", NULL);
        if ( format != NULL )
        {
            g_free(context.format);
            context.format = format;
        }
        g_key_file_unref(key_file);
    }

    context.section = g_new0(J4statusSection, 1);
    context.section->name = "time";
    context.section->state = J4STATUS_STATE_NO_STATE;
    context.section->value = g_malloc0(sizeof(char) * (TIME_SIZE + 1) );

    _j4status_time_update(NULL);

    context.sections = g_list_prepend(context.sections, context.section);
    g_timeout_add_seconds(1, _j4status_time_update, NULL);

    return &context.sections;
}
