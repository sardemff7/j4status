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
#include <gio/gio.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

static struct {
    GList *sections;
} context;

static void
_j4status_file_monitor_changed(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
    J4statusSection *section = user_data;

    GFileInputStream *stream;
    stream = g_file_read(file, NULL, NULL);
    if ( stream != NULL )
    {
        GDataInputStream *data_stream;
        data_stream = g_data_input_stream_new(G_INPUT_STREAM(stream));
        g_object_unref(stream);
        gsize length;
        g_free(section->value);
        section->value = g_data_input_stream_read_upto(data_stream, "", -1, &length, NULL, NULL);
        section->dirty = TRUE;
    }
}

GList **
j4status_input()
{
    context.sections = NULL;

    gchar *dir;
    dir = g_build_filename(g_get_user_runtime_dir(), PACKAGE_NAME G_DIR_SEPARATOR_S "file-monitor", NULL);
    if ( ( ! g_file_test(dir, G_FILE_TEST_IS_DIR) ) && ( g_mkdir_with_parents(dir, 0755) < 0 ) )
        g_warning("Couldn't create the directory to monitor '%s': %s", dir, g_strerror(errno));
    else
    {

        GKeyFile *key_file;
        key_file = libj4status_config_get_key_file("FileMonitor");
        if ( key_file != NULL )
        {
            gchar **files;
            files = g_key_file_get_string_list(key_file, "FileMonitor", "Files", NULL, NULL);
            if ( files != NULL )
            {
                gchar **file;
                for ( file = files ; *file != NULL ; ++file )
                {
                    GFile *g_file;
                    GFileMonitor *monitor;

                    if ( g_path_is_absolute(*file) )
                        g_file = g_file_new_for_path(*file);
                    else
                    {
                        gchar *filename;
                        filename = g_build_filename(dir, *file, NULL);

                        g_file = g_file_new_for_path(filename);
                        g_free(filename);
                    }
                    monitor = g_file_monitor_file(g_file, G_FILE_MONITOR_NONE, NULL, NULL);
                    g_object_unref(g_file);
                    if ( monitor == NULL )
                    {
                        g_free(*file);
                        continue;
                    }

                    J4statusSection *section;
                    section = g_new0(J4statusSection, 1);
                    section->name = "file-monitor";
                    section->instance = *file;
                    section->label = *file;
                    section->dirty = TRUE;
                    g_signal_connect(monitor, "changed", G_CALLBACK(_j4status_file_monitor_changed), section);
                    context.sections = g_list_prepend(context.sections, section);
                }
                g_free(files);
            }
            g_key_file_unref(key_file);
        }
    }
    g_free(dir);
    context.sections = g_list_reverse(context.sections);
    return &context.sections;
}
