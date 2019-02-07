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
#include <gio/gio.h>

#include <j4status-plugin-input.h>

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *sections;
};

typedef struct {
    J4statusPluginContext *context;
    GFileMonitor *monitor;
    J4statusSection *section;
} J4statusFileMonitorSection;

static void
_j4status_file_monitor_changed(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
    J4statusFileMonitorSection *section = user_data;

    GFileInputStream *stream;
    stream = g_file_read(file, NULL, NULL);
    if ( stream != NULL )
    {
        GDataInputStream *data_stream;
        data_stream = g_data_input_stream_new(G_INPUT_STREAM(stream));
        g_object_unref(stream);
        gsize length;
        j4status_section_set_value(section->section, g_data_input_stream_read_upto(data_stream, "", -1, &length, NULL, NULL));
    }
}

static void
_j4status_file_monitor_section_free(gpointer data)
{
    J4statusFileMonitorSection *section = data;

    j4status_section_free(section->section);

    g_object_unref(section->monitor);

    g_free(section);
}

static J4statusPluginContext *
_j4status_file_monitor_init(J4statusCoreInterface *core)
{
    GKeyFile *key_file = NULL;

    gchar *dir;
    dir = g_build_filename(g_get_user_runtime_dir(), PACKAGE_NAME G_DIR_SEPARATOR_S "file-monitor", NULL);
    if ( ( ! g_file_test(dir, G_FILE_TEST_IS_DIR) ) && ( g_mkdir_with_parents(dir, 0755) < 0 ) )
    {
        g_warning("Couldn't create the directory to monitor '%s': %s", dir, g_strerror(errno));
        goto fail;
    }
    key_file = j4status_config_get_key_file("FileMonitor");
    if ( key_file == NULL )
    {
        g_message("Missing configuration: No section, aborting");
        goto fail;
    }

    gchar **files;
    files = g_key_file_get_string_list(key_file, "FileMonitor", "Files", NULL, NULL);
    if ( files == NULL )
    {
        g_message("Missing configuration: Empty list of files to monitor, aborting");
        goto fail;
    }

    g_key_file_free(key_file);

    J4statusPluginContext *context;
    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    gchar **file;
    for ( file = files ; *file != NULL ; ++file )
    {
        GError *error = NULL;
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
        monitor = g_file_monitor_file(g_file, G_FILE_MONITOR_NONE, NULL, &error);
        g_object_unref(g_file);
        if ( monitor == NULL )
        {
            g_warning("Couldn't monitor file '%s': %s", *file, error->message);
            g_clear_error(&error);
            continue;
        }

        J4statusFileMonitorSection *section;
        section = g_new0(J4statusFileMonitorSection, 1);
        section->context = context;
        section->monitor = monitor;
        section->section = j4status_section_new(context->core);

        j4status_section_set_name(section->section, "file-monitor");
        j4status_section_set_instance(section->section, *file);
        j4status_section_set_label(section->section, *file);
        g_signal_connect(monitor, "changed", G_CALLBACK(_j4status_file_monitor_changed), section);

        if ( j4status_section_insert(section->section) )
            context->sections = g_list_prepend(context->sections, section);
        else
            _j4status_file_monitor_section_free(section);
    }
    g_strfreev(files);
    g_free(dir);

    if ( context->sections == NULL )
    {
        g_free(context);
        g_free(dir);
        return NULL;
    }

    return context;

fail:
    if ( key_file != NULL )
        g_key_file_free(key_file);
    g_free(dir);
    return NULL;
}

static void
_j4status_file_monitor_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_file_monitor_section_free);

    g_free(context);
}

J4STATUS_EXPORT void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_file_monitor_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_file_monitor_uninit);
}
