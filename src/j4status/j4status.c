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
#include <glib-object.h>
#include <glib/gprintf.h>
#ifdef G_OS_UNIX
#include <glib-unix.h>
#endif /* G_OS_UNIX */
#include <gio/gio.h>

#include <j4status-plugin.h>

#include "plugins.h"

static GMainLoop *loop = NULL;

void
j4status_core_quit()
{
    if ( loop != NULL )
        g_main_loop_quit(loop);
}

#if DEBUG
static void
_j4status_core_debug_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    GDataOutputStream *debug_stream = user_data;

    g_log_default_handler(log_domain, log_level, message, NULL);

    gchar *full_message;
    gchar *log_domain_message = NULL;
    const gchar *log_level_message = "";

    if ( log_domain != NULL )
        log_domain_message = g_strconcat(" [", log_domain, "]", NULL);

    switch ( log_level & G_LOG_LEVEL_MASK )
    {
        case G_LOG_LEVEL_ERROR:
            log_level_message = "ERROR";
        break;
        case G_LOG_LEVEL_CRITICAL:
            log_level_message = "CRITICAL";
        break;
        case G_LOG_LEVEL_WARNING:
            log_level_message = "WARNING";
        break;
        case G_LOG_LEVEL_MESSAGE:
            log_level_message = "MESSAGE";
        break;
        case G_LOG_LEVEL_INFO:
            log_level_message = "INFO";
        break;
        case G_LOG_LEVEL_DEBUG:
            log_level_message = "DEBUG";
        break;
    }

    full_message = g_strconcat(log_level_message, ( log_domain_message != NULL ) ? log_domain_message : "", ": ", message, "\n", NULL);

    g_data_output_stream_put_string(debug_stream, full_message, NULL, NULL);

    g_free(log_domain_message);
    g_free(full_message);
}

#endif /* ! DEBUG */

#ifdef G_OS_UNIX
static gboolean
_j4status_core_quit(gpointer user_data)
{
    j4status_core_quit();
    return FALSE;
}
#endif /* G_OS_UNIX */

static GList *sections = NULL;
static J4statusOutputFunc output_func = NULL;

static gboolean
_j4status_timeout_function(gpointer user_data)
{
    output_func(sections);
    fflush(stdout);
    return TRUE;
}

int
main(int argc, char *argv[])
{
    gboolean print_version = FALSE;
    gchar **input_plugins = NULL;
    gchar *output_plugin = NULL;
    guint interval = 1;

    int retval = 0;
    GError *error = NULL;
    GOptionContext *option_context = NULL;
    GOptionGroup *option_group;

#if DEBUG
    g_setenv("G_MESSAGES_DEBUG", "all", FALSE);
#endif /* ! DEBUG */

#if ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

    g_type_init();

#if DEBUG
    const gchar *debug_log_filename =  g_getenv("EVENTD_DEBUG_LOG_FILENAME");
    GDataOutputStream *debug_stream = NULL;

    if ( debug_log_filename != NULL )
    {
        GFile *debug_log;

        debug_log = g_file_new_for_path(debug_log_filename);

        GError *error = NULL;
        GFileOutputStream *debug_log_stream;

        debug_log_stream = g_file_append_to(debug_log, G_FILE_CREATE_NONE, NULL, &error);

        if ( debug_log_stream == NULL )
        {
            g_warning("Couldn't open debug log file: %s", error->message);
            g_clear_error(&error);
        }
        else
        {
            debug_stream = g_data_output_stream_new(G_OUTPUT_STREAM(debug_log_stream));
            g_object_unref(debug_log_stream);

            g_log_set_default_handler(_j4status_core_debug_log_handler, debug_stream);
        }
        g_object_unref(debug_log);
    }
#endif /* DEBUG */

    GOptionEntry entries[] =
    {
        { "interval", 'I', 0, G_OPTION_ARG_INT,          &interval,      "Interval between each output", "<seconds>" },
        { "input",    'i', 0, G_OPTION_ARG_STRING_ARRAY, &input_plugins, "Input plugins to use (may be specified several times)", "<plugin>" },
        { "output",   'o', 0, G_OPTION_ARG_STRING,       &output_plugin, "Output plugin to use", "<plugin>" },
        { "version",  'V', 0, G_OPTION_ARG_NONE,         &print_version, "Print version",        NULL },
        { NULL }
    };


    option_context = g_option_context_new("- status line generator");

    option_group = g_option_group_new(NULL, NULL, NULL, NULL, NULL);
    g_option_group_set_translation_domain(option_group, GETTEXT_PACKAGE);
    g_option_group_add_entries(option_group, entries);
    g_option_context_set_main_group(option_context, option_group);

    if ( ! g_option_context_parse(option_context, &argc, &argv, &error) )
        g_error("Option parsing failed: %s\n", error->message);
    g_option_context_free(option_context);

    if ( print_version )
    {
        g_fprintf(stdout, PACKAGE_NAME " " PACKAGE_VERSION "\n");
        goto end;
    }

    J4statusOutputInitFunc output_init_func;
    output_init_func = j4status_plugins_get_output_init_func(output_plugin);
    output_func = j4status_plugins_get_output_func(output_plugin);
    sections = j4status_plugins_get_sections(input_plugins);

    if ( output_init_func != NULL )
    {
        output_init_func();
        fflush(stdout);
    }

#ifdef G_OS_UNIX
    g_unix_signal_add(SIGTERM, _j4status_core_quit, NULL);
    g_unix_signal_add(SIGINT, _j4status_core_quit, NULL);
#endif /* G_OS_UNIX */

    g_timeout_add_seconds(interval, _j4status_timeout_function, NULL);
    output_func(sections);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

end:
#if DEBUG
    if ( debug_stream != NULL )
        g_object_unref(debug_stream);
#endif /* DEBUG */

    return retval;
}
