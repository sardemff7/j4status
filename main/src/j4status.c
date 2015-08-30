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
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */

#include <glib.h>
#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#endif /* ENABLE_NLS */
#include <glib-object.h>
#include <glib/gstdio.h>
#ifdef G_OS_UNIX
#include <glib-unix.h>
#endif /* G_OS_UNIX */
#include <gio/gio.h>

#include <j4status-plugin-output.h>
#include <j4status-plugin-input.h>
#include <j4status-plugin-private.h>

#include "plugins.h"

struct _J4statusCoreContext {
    guint interval;
    GMainLoop *loop;
    GList *input_plugins;
    GHashTable *order_weights;
    GList *sections;
    GHashTable *sections_hash;
    J4statusOutputPlugin *output_plugin;
    gboolean started;
    gulong display_handle;
    gboolean should_display;
};

#if DEBUG
static void
_j4status_core_debug_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    GDataOutputStream *stream = user_data;

    g_log_default_handler(log_domain, log_level, message, NULL);

    const gchar *prg_name;
    gchar pid[128];
    prg_name = g_get_prgname();
    g_sprintf(pid, "%lu", (gulong) getpid());

    g_data_output_stream_put_string(stream, "(", NULL, NULL);
    if ( prg_name != NULL )
    {
        g_data_output_stream_put_string(stream, prg_name, NULL, NULL);
        g_data_output_stream_put_string(stream, ":", NULL, NULL);
    }
    else
        g_data_output_stream_put_string(stream, "process:", NULL, NULL);
    g_data_output_stream_put_string(stream, pid, NULL, NULL);
    g_data_output_stream_put_string(stream, ") ", NULL, NULL);

    const gchar *log_level_message = "";
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
    g_data_output_stream_put_string(stream, log_level_message, NULL, NULL);

    if ( log_domain != NULL )
    {
        g_data_output_stream_put_string(stream, " [", NULL, NULL);
        g_data_output_stream_put_string(stream, log_domain, NULL, NULL);
        g_data_output_stream_put_string(stream, "]", NULL, NULL);
    }

    g_data_output_stream_put_string(stream, ": ", NULL, NULL);
    g_data_output_stream_put_string(stream, message, NULL, NULL);
    g_data_output_stream_put_byte(stream, '\n', NULL, NULL);
}

#endif /* ! DEBUG */

static gint
_j4status_core_compare_sections(gconstpointer a_, gconstpointer b_)
{
    const J4statusSection *a = a_, *b = b_;
    return (a->weight - b->weight);
}

static gboolean
_j4status_core_add_section(J4statusCoreContext *context, J4statusSection *section)
{
    if ( g_hash_table_lookup_extended(context->sections_hash, section->id, NULL, NULL) )
        return FALSE;

    g_hash_table_insert(context->sections_hash, section->id, section);

    if ( context->order_weights != NULL )
    {
        section->weight = GPOINTER_TO_INT(g_hash_table_lookup(context->order_weights, section->id));
        if ( section->weight == 0 )
            section->weight = GPOINTER_TO_INT(g_hash_table_lookup(context->order_weights, section->name));
    }
    if ( context->loop == NULL )
    {
        /* We are not started, thus sort has yet to happen */
        context->sections = g_list_prepend(context->sections, section);
        section->link = context->sections;
    }
    else
    {
        context->sections = g_list_insert_sorted(context->sections, section, _j4status_core_compare_sections);
        section->link = g_list_find(context->sections, section);
    }
    return TRUE;
}

void
_j4status_core_remove_section(J4statusCoreContext *context, J4statusSection *section)
{
    context->sections = g_list_remove_link(context->sections, section->link);
    g_hash_table_remove(context->sections_hash, section->id);
}

static gboolean
_j4status_core_display(gpointer user_data)
{
    J4statusCoreContext *context = user_data;

    context->display_handle = 0;
    context->should_display = FALSE;

    gchar *line;
    line = context->output_plugin->interface.generate(context->output_plugin->context, context->sections);
    g_printf("%s", line);
    g_free(line);
    fflush(stdout);


    return FALSE;
}

static void
_j4status_core_trigger_display(J4statusCoreContext *context, gboolean force)
{
    if ( context->display_handle > 0 )
        return;

    if ( context->started || force )
        context->display_handle = g_idle_add(_j4status_core_display, context);
    else
        context->should_display = TRUE;
}

static void
_j4status_core_trigger_action(J4statusCoreContext *context, const gchar *section_id, const gchar *event_id)
{
    J4statusSection *section;
    section = g_hash_table_lookup(context->sections_hash, section_id);
    if ( section == NULL )
        return;

    if ( section->action.callback == NULL )
        return;

    section->action.callback(section, event_id, section->action.user_data);
}

static void
_j4status_core_start(J4statusCoreContext *context)
{
    context->started = TRUE;

    GList *input_plugin_;
    J4statusInputPlugin *input_plugin;
    for ( input_plugin_ = context->input_plugins ; input_plugin_ != NULL ; input_plugin_ = g_list_next(input_plugin_) )
    {
        input_plugin = input_plugin_->data;
        if ( input_plugin->interface.start != NULL )
            input_plugin->interface.start(input_plugin->context);
    }
    if ( context->should_display && ( context->display_handle == 0 ) )
        context->display_handle = g_idle_add(_j4status_core_display, context);
}

static void
_j4status_core_stop(J4statusCoreContext *context)
{
    GList *input_plugin_;
    J4statusInputPlugin *input_plugin;
    for ( input_plugin_ = context->input_plugins ; input_plugin_ != NULL ; input_plugin_ = g_list_next(input_plugin_) )
    {
        input_plugin = input_plugin_->data;
        if ( input_plugin->interface.stop != NULL )
            input_plugin->interface.stop(input_plugin->context);
    }

    context->started = FALSE;
}

static void
_j4status_core_quit(J4statusCoreContext *context)
{
    if ( context->started )
        _j4status_core_stop(context);

    if ( context->loop != NULL )
        g_main_loop_quit(context->loop);
}

static gboolean
_j4status_core_source_quit(gpointer user_data)
{
    _j4status_core_quit(user_data);
    return FALSE;
}

#ifdef G_OS_UNIX
static gboolean
_j4status_core_signal_int(gpointer user_data)
{
    if ( isatty(0) )
        _j4status_core_quit(user_data);
    else
        _j4status_core_stop(user_data);
    return TRUE;
}

static gboolean
_j4status_core_signal_hup(gpointer user_data)
{
    J4statusCoreContext *context = user_data;
    if ( context->started )
        _j4status_core_stop(context);
    else
        _j4status_core_start(context);
    return TRUE;
}
#endif /* G_OS_UNIX */

int
main(int argc, char *argv[])
{
    gboolean print_version = FALSE;
    gboolean one_shot = FALSE;
    gchar *output_plugin = NULL;
    gchar **input_plugins = NULL;
    gchar **order = NULL;
    gchar *config = NULL;

    int retval = 0;
    GError *error = NULL;
    GOptionContext *option_context = NULL;
    GOptionGroup *option_group;

#if DEBUG
    g_setenv("G_MESSAGES_DEBUG", "all", FALSE);
#endif /* ! DEBUG */

    setlocale(LC_ALL, "");
#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

#if ! GLIB_CHECK_VERSION(2,35,0)
    g_type_init();
#endif /* ! GLIB_CHECK_VERSION(2,35,0) */

#if DEBUG
    const gchar *debug_log_filename =  g_getenv("J4STATUS_DEBUG_LOG_FILENAME");
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
        { "output",   'o', 0, G_OPTION_ARG_STRING,       &output_plugin, "Output plugin to use", "<plugin>" },
        { "input",    'i', 0, G_OPTION_ARG_STRING_ARRAY, &input_plugins, "Input plugins to use (may be specified several times)", "<plugin>" },
        { "order",    'O', 0, G_OPTION_ARG_STRING_ARRAY, &order,         "Order of sections, specified once a section (see man)", "<section id>" },
        { "one-shot", '1', 0, G_OPTION_ARG_NONE,         &one_shot,      "Tells j4status to stop right after starting",           NULL },
        { "config",   'c', 0, G_OPTION_ARG_STRING,       &config,        "Config file to use", "<config>" },
        { "version",  'V', 0, G_OPTION_ARG_NONE,         &print_version, "Print version",        NULL },
        { NULL }
    };


    option_context = g_option_context_new("- status line generator");

    option_group = g_option_group_new(NULL, NULL, NULL, NULL, NULL);
    g_option_group_set_translation_domain(option_group, GETTEXT_PACKAGE);
    g_option_group_add_entries(option_group, entries);
    g_option_context_set_main_group(option_context, option_group);

    if ( ! g_option_context_parse(option_context, &argc, &argv, &error) )
    {
        g_warning("Option parsing failed: %s\n", error->message);
        g_clear_error(&error);
        retval = 1;
        goto end;
    }
    g_option_context_free(option_context);

    if ( print_version )
    {
        g_fprintf(stdout, PACKAGE_NAME " " PACKAGE_VERSION "\n");
        goto end;
    }

    if ( config != NULL )
    {
        g_setenv("J4STATUS_CONFIG_FILE", config, TRUE);
        g_free(config);
    }

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("Plugins");
    if ( key_file != NULL )
    {
        if ( output_plugin == NULL )
            output_plugin = g_key_file_get_string(key_file, "Plugins", "Output", NULL);

        if ( input_plugins == NULL )
            input_plugins = g_key_file_get_string_list(key_file, "Plugins", "Input", NULL, NULL);

        if ( order == NULL )
            order = g_key_file_get_string_list(key_file, "Plugins", "Order", NULL, NULL);

        g_key_file_free(key_file);
    }

    J4statusCoreContext *context;
    context = g_new0(J4statusCoreContext, 1);

    J4statusCoreInterface interface = {
        .context = context,
        .add_section = _j4status_core_add_section,
        .remove_section = _j4status_core_remove_section,
        .trigger_display = _j4status_core_trigger_display,
        .trigger_action = _j4status_core_trigger_action,
    };

#ifdef G_OS_UNIX
    g_unix_signal_add(SIGTERM, _j4status_core_source_quit, context);

    g_unix_signal_add(SIGINT, _j4status_core_signal_int, context);
    g_unix_signal_add(SIGHUP, _j4status_core_signal_hup, context);

    /* Ignore SIGPIPE as it is useless */
    signal(SIGPIPE, SIG_IGN);
#endif /* G_OS_UNIX */

    context->output_plugin = j4status_plugins_get_output_plugin(&interface, output_plugin);
    if ( context->output_plugin == NULL )
    {
        g_warning("No usable output plugin, tried '%s'", output_plugin);
        retval = 10;
        goto end;
    }

    if ( order != NULL )
    {
        context->order_weights = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        gchar **id;
        for ( id = order ; *id != NULL ; ++id )
            g_hash_table_insert(context->order_weights, *id, GINT_TO_POINTER(1 + id - order));
        g_free(order);
    }

    context->sections_hash = g_hash_table_new(g_str_hash, g_str_equal);

    context->input_plugins = j4status_plugins_get_input_plugins(&interface, input_plugins);
    if ( context->input_plugins == NULL )
    {
        g_warning("No input plugins, will stop early");
        one_shot = TRUE;
        retval = 11;
    }
    context->sections = g_list_reverse(context->sections);
    if ( context->order_weights != NULL )
        context->sections = g_list_sort(context->sections, _j4status_core_compare_sections);

    _j4status_core_start(context);

    if ( one_shot )
        g_idle_add(_j4status_core_source_quit, context);

    context->loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(context->loop);
    g_main_loop_unref(context->loop);

    GList *input_plugin_;
    J4statusInputPlugin *input_plugin;
    for ( input_plugin_ = context->input_plugins ; input_plugin_ != NULL ; input_plugin_ = g_list_next(input_plugin_) )
    {
        input_plugin = input_plugin_->data;
        input_plugin->interface.uninit(input_plugin->context);
    }

    if ( context->output_plugin->interface.uninit != NULL )
    {
        context->output_plugin->interface.uninit(context->output_plugin->context);
        fflush(stdout);
    }

    if ( context->order_weights != NULL )
        g_hash_table_unref(context->order_weights);

    g_hash_table_unref(context->sections_hash);

end:
#if DEBUG
    if ( debug_stream != NULL )
        g_object_unref(debug_stream);
#endif /* DEBUG */

    return retval;
}
