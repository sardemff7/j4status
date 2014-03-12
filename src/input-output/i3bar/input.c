/*
 * j4status - Status line generator
 *
 * Copyright © 2012-2014 Quentin "Sardem FF7" Glidic
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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <gio/gio.h>
#ifdef G_OS_UNIX
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#endif /* G_OS_UNIX */

#include <yajl/yajl_parse.h>

#include <j4status-plugin-input.h>
#include <j4status-plugin-output.h>

#include <libj4status-config.h>

#ifdef G_OS_UNIX
#define stream_from_fd(type, fd) g_unix_ ## type ## put_stream_new(fd, TRUE)
#else /* ! G_OS_UNIX */
#error No non-UNIX support!
#endif /* ! G_OS_UNIX */

#define yajl_strcmp(str1, len1, str2) ( ( strlen(str2) == len1 ) && ( g_ascii_strncasecmp((const gchar *) str1, str2, len1) == 0 ) )

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *clients;
};

typedef enum {
    /* Header keys */
    KEY_VERSION      = (1<<0),
    KEY_STOP_SIGNAL,
    KEY_CONT_SIGNAL,
    KEY_CLICK_EVENTS,

    /* Sections keys */
    KEY_NAME         = (1<<4),
    KEY_INSTANCE,
    KEY_FULL_TEXT,
    KEY_SHORT_TEXT,
    KEY_URGENT,
    KEY_COLOUR,
    KEY_ALIGN,
    KEY_MIN_WIDTH,
    KEY_SEPARATOR,
    KEY_SEPARATOR_BLOCK_WIDTH,

    KEY_NONE = 0
} J4statusI3barInputJsonKey;

static gchar *_j4status_i3bar_input_json_key_names[] = {
    [KEY_NONE] = "none",

    [KEY_VERSION] = "version",
    [KEY_STOP_SIGNAL] = "stop_signal",
    [KEY_CONT_SIGNAL] = "cont_signal",
    [KEY_CLICK_EVENTS] = "click_events",

    [KEY_NAME] = "name",
    [KEY_INSTANCE] = "instance",
    [KEY_FULL_TEXT] = "full_text",
    [KEY_SHORT_TEXT] = "short_text",
    [KEY_URGENT] = "urgent",
    [KEY_COLOUR] = "colour",
    [KEY_ALIGN] = "align",
    [KEY_MIN_WIDTH] = "min_width",
    [KEY_SEPARATOR] = "separator",
    [KEY_SEPARATOR_BLOCK_WIDTH] = "separator_block_width",
};

typedef struct {
    gchar *error;
    J4statusI3barInputJsonKey key;
    gint64 version;
    gint stop_signal;
    gint cont_signal;
    gboolean click_events;
} J4statusI3barInputHeaderParseContext;

typedef struct {
    gchar *error;
    guint array_nesting;
    gboolean in_section;
    J4statusI3barInputJsonKey key;
    gchar *name;
    gchar *instance;
    gchar *full_text;
    gboolean urgent;
    J4statusColour colour;
    J4statusAlign align;
    gint64 max_width;
    gboolean separator;
    gint64 separator_width;
} J4statusI3barSectionParseContext;

typedef struct {
    J4statusPluginContext *context;
    GList *link;
    gchar *name;
    GPid pid;
    GDataOutputStream *stdin;
    GDataInputStream *stdout;
    GDataInputStream *stderr;
    GCancellable *cancellable;
    gint stop_signal;
    gint cont_signal;
    yajl_handle json_handle;
    J4statusI3barSectionParseContext parse_context;
    GHashTable *sections;
} J4statusI3barInputClient;

/* Header parsing */

static int
_j4status_i3bar_input_header_boolean(void *user_data, int value)
{
    J4statusI3barInputHeaderParseContext *context = user_data;

    switch ( context->key )
    {
    case KEY_CLICK_EVENTS:
        context->click_events = value;
    break;
    default:
        context->error = g_strdup_printf("Wrong boolean key '%s'",
            _j4status_i3bar_input_json_key_names[context->key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_header_integer(void *user_data, long long value)
{
    J4statusI3barInputHeaderParseContext *context = user_data;

    switch ( context->key )
    {
    case KEY_VERSION:
        context->version = value;
    break;
    case KEY_STOP_SIGNAL:
        context->stop_signal = value;
    break;
    case KEY_CONT_SIGNAL:
        context->cont_signal = value;
    break;
    default:
        context->error = g_strdup_printf("Wrong integer key '%s'",
            _j4status_i3bar_input_json_key_names[context->key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_header_map_key(void *user_data, const unsigned char *value, size_t length)
{
    J4statusI3barInputHeaderParseContext *context = user_data;

    if ( yajl_strcmp(value, length, "version") )
        context->key = KEY_VERSION;
    else if ( yajl_strcmp(value, length, "stop_signal") )
        context->key = KEY_STOP_SIGNAL;
    else if ( yajl_strcmp(value, length, "cont_signal") )
        context->key = KEY_CONT_SIGNAL;
    else if ( yajl_strcmp(value, length, "click_events") )
        context->key = KEY_CLICK_EVENTS;
    else
    {
        context->error = g_strdup_printf("Wrong key '%.*s'", (gint) length, value);
        return 0;
    }

    return 1;
}

static yajl_callbacks _j4status_i3bar_input_header_callbacks = {
    .yajl_boolean = _j4status_i3bar_input_header_boolean,
    .yajl_integer = _j4status_i3bar_input_header_integer,
    .yajl_map_key = _j4status_i3bar_input_header_map_key,
};

/* Section parsing */

static int
_j4status_i3bar_input_section_boolean(void *user_data, int value)
{
    J4statusI3barInputClient *client = user_data;

    if ( ! client->parse_context.in_section )
    {
        if ( client->parse_context.key != KEY_NONE )
            client->parse_context.error = g_strdup_printf("Key '%s' must be in a section",
                _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        else
            client->parse_context.error = g_strdup_printf("Unexpected boolean value: %s", value ? "true" : "false");
        return 0;
    }

    switch ( client->parse_context.key )
    {
    case KEY_URGENT:
        client->parse_context.urgent = value;
    break;
    case KEY_SEPARATOR:
        /* Ignoring */
    break;
    default:
        client->parse_context.error = g_strdup_printf("Wrong boolean key '%s'",
            _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_section_integer(void *user_data, long long value)
{
    J4statusI3barInputClient *client = user_data;

    if ( ! client->parse_context.in_section )
    {
        if ( client->parse_context.key != KEY_NONE )
            client->parse_context.error = g_strdup_printf("Key '%s' must be in a section",
                _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        else
            client->parse_context.error = g_strdup_printf("Unexpected integer value: %lld", value);
        return 0;
    }

    switch ( client->parse_context.key )
    {
    case KEY_MIN_WIDTH:
        client->parse_context.max_width = value;
    break;
    case KEY_SEPARATOR_BLOCK_WIDTH:
        /* Ignoring */
    break;
    default:
        client->parse_context.error = g_strdup_printf("Wrong integer key '%s'",
            _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_section_string(void *user_data, const unsigned char *value, size_t length)
{
    J4statusI3barInputClient *client = user_data;

    if ( ! client->parse_context.in_section )
    {
        if ( client->parse_context.key != KEY_NONE )
            client->parse_context.error = g_strdup_printf("Key '%s' must be in a section",
                _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        else
            client->parse_context.error = g_strdup_printf("Unexpected string value: %.*s", (gint) length, value);
        return 0;
    }

    switch ( client->parse_context.key )
    {
    case KEY_NAME:
        client->parse_context.name = g_strndup((const gchar *) value, length);
    break;
    case KEY_INSTANCE:
        client->parse_context.instance = g_strndup((const gchar *) value, length);
    break;
    case KEY_FULL_TEXT:
        client->parse_context.full_text = g_strndup((const gchar *) value, length);
    break;
    case KEY_SHORT_TEXT:
        /* Ignoring */
    break;
    case KEY_COLOUR:
        client->parse_context.colour = j4status_colour_parse_length((const gchar *) value, length);
    break;
    case KEY_ALIGN:
        if ( yajl_strcmp(value, length, "left") )
            client->parse_context.align = J4STATUS_ALIGN_LEFT;
        else if ( yajl_strcmp(value, length, "left") )
            client->parse_context.align = J4STATUS_ALIGN_RIGHT;
        else if ( yajl_strcmp(value, length, "centor") )
            client->parse_context.align = J4STATUS_ALIGN_CENTER;
        else
            return 0;
    break;
    case KEY_MIN_WIDTH:
        client->parse_context.max_width = -length;
    break;
    default:
        client->parse_context.error = g_strdup_printf("Wrong string key '%s'",
            _j4status_i3bar_input_json_key_names[client->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_section_start_map(void *user_data)
{
    J4statusI3barInputClient *client = user_data;

    if ( client->parse_context.in_section )
    {
        client->parse_context.error = g_strdup_printf("Unexpected map in section");
        return 0;
    }

    client->parse_context.in_section = TRUE;

    return 1;
}

static int
_j4status_i3bar_input_section_map_key(void *user_data, const unsigned char *value, size_t length)
{
    J4statusI3barInputClient *client = user_data;

    if ( ! client->parse_context.in_section )
    {
        client->parse_context.error = g_strdup_printf("Unexpected map key outside section: %.*s", (gint) length, value);
        return 0;
    }

    if ( yajl_strcmp(value, length, "name") )
        client->parse_context.key = KEY_NAME;
    else if ( yajl_strcmp(value, length, "instance") )
        client->parse_context.key = KEY_INSTANCE;
    else if ( yajl_strcmp(value, length, "full_text") )
        client->parse_context.key = KEY_FULL_TEXT;
    else if ( yajl_strcmp(value, length, "short_text") )
        client->parse_context.key = KEY_SHORT_TEXT;
    else if ( yajl_strcmp(value, length, "urgent") )
        client->parse_context.key = KEY_URGENT;
    else if ( yajl_strcmp(value, length, "color") )
        client->parse_context.key = KEY_COLOUR;
    else if ( yajl_strcmp(value, length, "align") )
        client->parse_context.key = KEY_ALIGN;
    else if ( yajl_strcmp(value, length, "min_width") )
        client->parse_context.key = KEY_MIN_WIDTH;
    else if ( yajl_strcmp(value, length, "separator") )
        client->parse_context.key = KEY_SEPARATOR;
    else if ( yajl_strcmp(value, length, "separator_block_width") )
        client->parse_context.key = KEY_SEPARATOR_BLOCK_WIDTH;
    else
    {
        client->parse_context.error = g_strdup_printf("Wrong key '%.*s'", (gint) length, value);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_section_end_map(void *user_data)
{
    J4statusI3barInputClient *client = user_data;

    if ( ! client->parse_context.in_section )
        return 0;

    const gchar *name = client->parse_context.name;
    const gchar *instance = client->parse_context.instance;
    if ( ( name == NULL ) && ( instance != NULL ) )
    {
        client->parse_context.error = g_strdup_printf("Section with instance but without name for client '%s': %s", client->name, instance);
        return 0;
    }
    if ( name == NULL )
    {
        name = "i3bar";
        if ( instance == NULL )
            instance = client->name;
    }

    J4statusSection *section;

    gsize l = strlen(name) + 1, o;
    if ( instance != NULL )
        l += strlen(":") + strlen(instance);

    gchar id[l];
    o = g_sprintf(id, "%s", name);
    if ( instance != NULL )
        g_sprintf(id + o, ":%s", instance);

    section = g_hash_table_lookup(client->sections, id);
    if ( section != NULL )
    {
        if ( j4status_section_get_align(section) != client->parse_context.align )
        {
            client->parse_context.error = g_strdup_printf("Section %s from client '%s': \"align\" mismatch", id, client->name);
            return 0;
        }
        if ( j4status_section_get_max_width(section) != client->parse_context.max_width )
        {
            client->parse_context.error = g_strdup_printf("Section %s from client '%s': \"min_width\" mismatch", id, client->name);
            return 0;
        }
    }
    else
    {
        section = j4status_section_new(client->context->core);

        j4status_section_set_name(section, name);
        j4status_section_set_instance(section, instance);
        j4status_section_set_align(section, client->parse_context.align);
        j4status_section_set_max_width(section, client->parse_context.max_width);

        if ( j4status_section_insert(section) )
            g_hash_table_insert(client->sections, g_strdup(id), section);
        else
        {
            j4status_section_free(section);
            goto end;
        }
    }

    J4statusState state = J4STATUS_STATE_NO_STATE;
    if ( client->parse_context.urgent )
        state |= J4STATUS_STATE_URGENT;

    j4status_section_set_state(section, state);
    j4status_section_set_value(section, client->parse_context.full_text);
    client->parse_context.full_text = NULL;
    j4status_section_set_colour(section, client->parse_context.colour);

end:
    client->parse_context.in_section = FALSE;

    client->parse_context.key = KEY_NONE;

    g_free(client->parse_context.name);
    client->parse_context.name = NULL;
    g_free(client->parse_context.instance);
    client->parse_context.instance = NULL;
    g_free(client->parse_context.full_text);
    client->parse_context.full_text = NULL;
    client->parse_context.urgent = FALSE;
    j4status_colour_reset(&client->parse_context.colour);
    client->parse_context.align = J4STATUS_ALIGN_CENTER;
    client->parse_context.max_width = 0;

    return 1;
}

static int
_j4status_i3bar_input_section_start_array(void *user_data)
{
    J4statusI3barInputClient *client = user_data;

    if ( ++client->parse_context.array_nesting > 2 )
    {
        client->parse_context.error = g_strdup_printf("Too much nested arrays: %u", client->parse_context.array_nesting);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_input_section_end_array(void *user_data)
{
    J4statusI3barInputClient *client = user_data;

    --client->parse_context.array_nesting;

    return 1;
}

static yajl_callbacks _j4status_i3bar_input_section_callbacks = {
    .yajl_boolean     = _j4status_i3bar_input_section_boolean,
    .yajl_integer     = _j4status_i3bar_input_section_integer,
    .yajl_string      = _j4status_i3bar_input_section_string,
    .yajl_start_map   = _j4status_i3bar_input_section_start_map,
    .yajl_map_key     = _j4status_i3bar_input_section_map_key,
    .yajl_end_map     = _j4status_i3bar_input_section_end_map,
    .yajl_start_array = _j4status_i3bar_input_section_start_array,
    .yajl_end_array   = _j4status_i3bar_input_section_end_array,
};

static void _j4status_i3bar_input_client_free(gpointer data);

static void
_j4status_i3bar_input_client_read_callback(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    J4statusI3barInputClient *client = user_data;
    GError *error = NULL;

    gchar *line;
    gsize length;
    line = g_data_input_stream_read_line_finish_utf8(client->stdout, res, &length, &error);
    if ( line == NULL )
    {
        if ( error == NULL )
            /* EOF, we keep things around, client will be freed later */
            return;

        g_warning("Couldn't read client '%s' output: %s", client->name, error->message);
        g_clear_error(&error);
        _j4status_i3bar_input_client_free(client);
        return;
    }

    yajl_status json_state;

    json_state = yajl_parse(client->json_handle, (const unsigned char *) line, length);

    if ( json_state != yajl_status_ok )
    {
        g_free(client->parse_context.name);
        g_free(client->parse_context.instance);
        g_free(client->parse_context.full_text);

        if ( json_state == yajl_status_error )
        {
            unsigned char *str_error;
            str_error = yajl_get_error(client->json_handle, 0, (const unsigned char *) line, length);
            g_warning("Couldn't parse section from client '%s': %s", client->name, str_error);
            yajl_free_error(client->json_handle, str_error);
        }
        else if ( json_state == yajl_status_client_canceled )
        {
            g_warning("i3bar JSON protocol section error from client '%s': %s", client->name, client->parse_context.error);
            g_free(client->parse_context.error);
        }

        _j4status_i3bar_input_client_free(client);
        return;
    }

    g_data_input_stream_read_line_async(client->stdout, G_PRIORITY_DEFAULT, client->cancellable, _j4status_i3bar_input_client_read_callback, client);
}

static J4statusI3barInputClient *
_j4status_i3bar_input_client_new(J4statusPluginContext *context, const gchar *client_command)
{
    GError *error = NULL;
    J4statusI3barInputClient *client = NULL;

    gchar **argv = NULL;
    if ( ! g_shell_parse_argv(client_command, NULL, &argv, &error) )
    {
        g_warning("Couldn't parse '%s': %s", client_command, error->message);
        goto fail;
    }

    client = g_new0(J4statusI3barInputClient, 1);
    client->context = context;
    client->name = g_path_get_basename(argv[0]);

    gint child_stdin_fd, child_stdout_fd, child_stderr_fd;
    if ( ! g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &client->pid, &child_stdin_fd, &child_stdout_fd, &child_stderr_fd, &error) )
    {
        g_warning("Couldn't spawn '%s': %s", client->name, error->message);
        goto fail;
    }

    GInputStream *raw_in;
    raw_in = stream_from_fd(in, child_stdout_fd);
    client->stdout = g_data_input_stream_new(raw_in);
    g_object_unref(raw_in);

    gchar *header;
    gsize length = 0;
    header = g_data_input_stream_read_line_utf8(client->stdout, &length, NULL, &error);
    if ( header == NULL )
    {
        g_warning("Couldn't read header from client '%s': %s", client->name, error->message);
        goto fail;
    }

    J4statusI3barInputHeaderParseContext header_context = {0};
    yajl_handle json_handle;
    yajl_status json_state;

    json_handle = yajl_alloc(&_j4status_i3bar_input_header_callbacks, NULL, &header_context);
    json_state = yajl_parse(json_handle, (const unsigned char *) header, length);

    if ( json_state != yajl_status_ok )
    {
        if ( json_state == yajl_status_error )
        {
            unsigned char *str_error;
            str_error = yajl_get_error(client->json_handle, 0, (const unsigned char *) header, length);
            g_warning("Couldn't parse header from client '%s': %s", client->name, str_error);
            yajl_free_error(client->json_handle, str_error);
        }
        else if ( json_state == yajl_status_client_canceled )
        {
            g_warning("i3bar JSON protocol header error from client '%s': %s", client->name, header_context.error);
            g_free(client->parse_context.error);
        }

        yajl_free(json_handle);
        goto fail;
    }

    client->stop_signal = header_context.stop_signal;
    client->cont_signal = header_context.cont_signal;

    if ( header_context.click_events )
    {
        GOutputStream *raw_out;
        raw_out = stream_from_fd(out, child_stdin_fd);
        client->stdin = g_data_output_stream_new(raw_out);
        g_object_unref(raw_out);
    }

    raw_in = stream_from_fd(in, child_stderr_fd);
    client->stderr = g_data_input_stream_new(raw_in);
    g_object_unref(raw_in);

    client->cancellable = g_cancellable_new();

    client->json_handle = yajl_alloc(&_j4status_i3bar_input_section_callbacks, NULL, client);

#ifdef G_OS_UNIX
    killpg(client->pid, client->stop_signal);
#endif /* G_OS_UNIX */

    g_data_input_stream_read_line_async(client->stdout, G_PRIORITY_DEFAULT, client->cancellable, _j4status_i3bar_input_client_read_callback, client);

    client->sections = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) j4status_section_free);

    return client;

fail:
    if ( client != NULL )
    {
        if ( client->stdin != NULL )
            g_object_unref(client->stdin);
        if ( client->stdout != NULL )
            g_object_unref(client->stdout);
        if ( client->stderr != NULL )
            g_object_unref(client->stderr);

        if ( client->pid != 0 )
            g_spawn_close_pid(client->pid);
        g_free(client->name);
    }
    g_free(client);

    g_strfreev(argv);

    g_clear_error(&error);
    return NULL;
}

static void
_j4status_i3bar_input_client_free(gpointer data)
{
    J4statusI3barInputClient *client = data;

    yajl_free(client->json_handle);

    g_object_unref(client->stderr);
    g_object_unref(client->stdout);
    if ( client->stdin != NULL )
        g_object_unref(client->stdin);

    g_spawn_close_pid(client->pid);

    g_free(client);
}

static void
_j4status_i3bar_input_client_start(gpointer data, gpointer user_data)
{
    J4statusI3barInputClient *client = data;

#ifdef G_OS_UNIX
    killpg(client->pid, client->cont_signal);
#endif /* G_OS_UNIX */
}

static void
_j4status_i3bar_input_client_stop(gpointer data, gpointer user_data)
{
    J4statusI3barInputClient *client = data;

#ifdef G_OS_UNIX
    killpg(client->pid, client->stop_signal);
#endif /* G_OS_UNIX */
}

static J4statusPluginContext *
_j4status_i3bar_input_init(J4statusCoreInterface *core)
{
    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("i3bar");
    if ( key_file == NULL )
        return NULL;

    gchar **clients;
    clients = g_key_file_get_string_list(key_file, "i3bar", "Clients", NULL, NULL);
    if ( clients == NULL )
    {
        g_key_file_free(key_file);
        return NULL;
    }
    g_key_file_free(key_file);

    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    gchar **client_command;
    for ( client_command = clients ; *client_command != NULL ; ++client_command )
    {
        J4statusI3barInputClient *client;
        client = _j4status_i3bar_input_client_new(context, *client_command);
        if ( client != NULL )
            client->link = context->clients = g_list_prepend(context->clients, client);
    }
    g_strfreev(clients);

    return context;
}

static void
_j4status_i3bar_input_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->clients, _j4status_i3bar_input_client_free);

    g_free(context);
}

static void
_j4status_i3bar_input_start(J4statusPluginContext *context)
{
    g_list_foreach(context->clients, _j4status_i3bar_input_client_start, context);
}

static void
_j4status_i3bar_input_stop(J4statusPluginContext *context)
{
    g_list_foreach(context->clients, _j4status_i3bar_input_client_stop, context);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_i3bar_input_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_i3bar_input_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_i3bar_input_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_i3bar_input_stop);
}
