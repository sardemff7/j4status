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

#include <glib.h>
#include <glib/gprintf.h>

#include <mpd/async.h>
#include <libgwater-mpd.h>

#include <j4status-plugin-input.h>

#define TIME_SIZE 4095

typedef enum {
    ACTION_TOGGLE,
    ACTION_PLAY,
    ACTION_PAUSE,
    ACTION_STOP,
    ACTION_NEXT,
    ACTION_PREVIOUS,
    ACTION_NONE
} J4statusMpdAction;

static const gchar * const _j4status_mpd_action_list[ACTION_NONE] = {
    [ACTION_TOGGLE]   = "toggle",
    [ACTION_PLAY]     = "play",
    [ACTION_PAUSE]    = "pause",
    [ACTION_STOP]     = "stop",
    [ACTION_NEXT]     = "next",
    [ACTION_PREVIOUS] = "previous",
};

typedef struct {
    GHashTable *actions;
} J4statusMpdConfig;

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    J4statusMpdConfig config;
    GList *sections;
    gboolean started;
};

typedef enum {
    COMMAND_IDLE,
    COMMAND_QUERY,
    COMMAND_ACTION,
} J4statusMpdCommand;

typedef enum {
    STATE_PLAY,
    STATE_PAUSE,
    STATE_STOP,
} J4statusMpdSectionState;

typedef struct {
    J4statusPluginContext *context;
    J4statusSection *section;
    GWaterMpdSource *source;
    struct mpd_async *mpd;

    guint64 used_tokens;
    J4statusFormatString *format;

    J4statusMpdCommand command;
    J4statusMpdAction pending;

    gchar *current_song;
    J4statusMpdSectionState state;
    gboolean updating;
    gboolean repeat;
    gboolean random;
    gboolean single;
    gboolean consume;
    gint8 volume;
} J4statusMpdSection;

typedef enum {
    TOKEN_SONG,
    TOKEN_STATE,
    TOKEN_DATABASE,
    TOKEN_OPTIONS,
    TOKEN_VOLUME,
    _TOKEN_SIZE
} J4statusMpdFormatToken;

typedef enum {
    TOKEN_FLAG_SONG     = (1 << TOKEN_SONG),
    TOKEN_FLAG_STATE    = (1 << TOKEN_STATE),
    TOKEN_FLAG_DATABASE = (1 << TOKEN_DATABASE),
    TOKEN_FLAG_OPTIONS  = (1 << TOKEN_OPTIONS),
    TOKEN_FLAG_VOLUME   = (1 << TOKEN_VOLUME),
} J4statusMpdFormatTokenFlag;

static const gchar * const _j4status_mpd_format_tokens[_TOKEN_SIZE] = {
    [TOKEN_SONG]     = "song",
    [TOKEN_STATE]    = "state",
    [TOKEN_DATABASE] = "database",
    [TOKEN_OPTIONS]  = "options",
    [TOKEN_VOLUME]   = "volume",
};

#define J4STATUS_MPD_DEFAULT_FORMAT "${song}${ (<database>)}${ [<options>]}"

static void
_j4status_mpd_section_command(J4statusMpdSection *section, J4statusMpdCommand command)
{
    if ( section->pending != ACTION_NONE )
        command = COMMAND_ACTION;

    switch ( command )
    {
    case COMMAND_IDLE:
    {
        const gchar *params[4] = {NULL};
        gsize n = 0;
        if ( section->used_tokens & (TOKEN_FLAG_STATE | TOKEN_FLAG_SONG) )
            params[n++] = "player";
        if ( section->used_tokens & TOKEN_FLAG_DATABASE )
            params[n++] = "database";
        if ( section->used_tokens & TOKEN_FLAG_OPTIONS )
            params[n++] = "options";
        if ( section->used_tokens & TOKEN_FLAG_VOLUME )
            params[n++] = "mixer";
        mpd_async_send_command(section->mpd, "idle", params[0], params[1], params[2], params[3], NULL);
    }
    break;
    case COMMAND_QUERY:
        mpd_async_send_command(section->mpd, "command_list_begin", NULL);
        mpd_async_send_command(section->mpd, "status", NULL);
        mpd_async_send_command(section->mpd, "currentsong", NULL);
        mpd_async_send_command(section->mpd, "command_list_end", NULL);
    break;
    case COMMAND_ACTION:
    {
        const gchar *command_str = "";
        const gchar *params[1] = {NULL};
        switch ( section->pending )
        {
        case ACTION_NONE:
            g_assert_not_reached();
        break;
        case ACTION_TOGGLE:
            command_str = "pause";
            params[0] = ( section->state == STATE_PAUSE ) ? "0" : "1";
        break;
        case ACTION_PLAY:
            command_str = "play";
        break;
        case ACTION_PAUSE:
            command_str = "pause";
            params[0] = "1";
        break;
        case ACTION_STOP:
            command_str = "stop";
        break;
        case ACTION_NEXT:
            command_str = "next";
        break;
        case ACTION_PREVIOUS:
            command_str = "previous";
        break;
        }
        mpd_async_send_command(section->mpd, command_str, params[0], NULL);
    }
    break;
    }
    section->command = command;
}

static void
_j4status_mpd_section_action_callback(J4statusSection *section_, const gchar *action_id, gpointer user_data)
{
    J4statusMpdSection *section = user_data;
    if ( section->pending != ACTION_NONE )
        return;

    switch ( section->command )
    {
    case COMMAND_IDLE:
        mpd_async_send_command(section->mpd, "noidle", NULL);
    break;
    case COMMAND_QUERY:
    break;
    case COMMAND_ACTION:
        /* Ignore */
        return;
    }
    section->pending = GPOINTER_TO_UINT(g_hash_table_lookup(section->context->config.actions, action_id));
}

const gchar *
_j4status_mpd_format_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    const J4statusMpdSection *section = user_data;
    static gchar options[5] = {0};
    static gchar volume[4] = {0};
    switch ( value )
    {
    case TOKEN_SONG:
        return ( section->current_song != NULL ) ? section->current_song : "No song";
    break;
    case TOKEN_STATE:
    {
        const gchar *status = NULL;
        switch ( section->state )
        {
        case STATE_PLAY:
        break;
        case STATE_PAUSE:
            status = "Paused";
        break;
        case STATE_STOP:
            status = "Stopped";
        break;
        }
        return status;
    }
    break;
    case TOKEN_DATABASE:
        return section->updating ? "Updating database" : NULL;
    break;
    case TOKEN_OPTIONS:
        options[0] = section->repeat ? 'r' : ' ';
        options[1] = section->random ? 'z' : ' ';
        options[2] = section->single ? '1' : ' ';
        options[3] = section->consume ? '-' : ' ';
        return options;
    break;
    case TOKEN_VOLUME:
        g_sprintf(volume, "%hd", section->volume);
        return volume;
    break;
    default:
        g_assert_not_reached();
    }
    return NULL;
}

static void
_j4status_mpd_section_update(J4statusMpdSection *section)
{
    J4statusState state = J4STATUS_STATE_NO_STATE;
    gchar *value;

    switch ( section->state )
    {
    case STATE_PLAY:
        state = J4STATUS_STATE_GOOD;
    break;
    case STATE_PAUSE:
        state = J4STATUS_STATE_AVERAGE;
    break;
    case STATE_STOP:
        state = J4STATUS_STATE_BAD;
    break;
    }

    value = j4status_format_string_replace(section->format, _j4status_mpd_format_callback, section);

    j4status_section_set_state(section->section, state);
    j4status_section_set_value(section->section, value);
}

static void _j4status_mpd_section_free(gpointer data);

static gboolean
_j4status_mpd_section_line_callback(gchar *line, enum mpd_error error, gpointer user_data)
{
    J4statusMpdSection *section = user_data;
    J4statusPluginContext *context = section->context;

    if ( error != MPD_ERROR_SUCCESS )
    {
        g_warning("MPD error: %s", mpd_async_get_error_message(section->mpd));
        goto fail;
    }

    switch ( section->command )
    {
    case COMMAND_IDLE:
        if ( g_str_has_prefix(line, "changed: ") )
        {
            const gchar *subsystem = line + strlen("changed: ");
            if ( g_strcmp0(subsystem, "database") == 0 )
                section->updating = FALSE;
            if ( g_strcmp0(subsystem, "player") == 0 )
            {
                g_free(section->current_song);
                section->current_song = NULL;
            }
            break;
        }
        if ( g_strcmp0(line, "OK") == 0 )
            _j4status_mpd_section_command(section, section->context->started ? COMMAND_QUERY : COMMAND_IDLE);
    break;
    case COMMAND_QUERY:
        if ( g_strcmp0(line, "OK") == 0 )
        {
            _j4status_mpd_section_update(section);
            _j4status_mpd_section_command(section, COMMAND_IDLE);
            break;
        }
        if ( g_str_has_prefix(line, "state: ") )
        {
            const gchar *state = line + strlen("state: ");
            if ( g_strcmp0(state, "play") == 0 )
                section->state = STATE_PLAY;
            else if ( g_strcmp0(state, "pause") == 0 )
                section->state = STATE_PAUSE;
            else if ( g_strcmp0(state, "stop") == 0 )
                section->state = STATE_STOP;
        }
        else if ( g_str_has_prefix(line, "updating_db: ") )
            section->updating = TRUE;
        else if ( g_ascii_strncasecmp(line, "Title: ", strlen("Title: ")) == 0 )
            section->current_song = g_strdup(line + strlen("Title: "));
        else if ( g_str_has_prefix(line, "repeat: ") )
            section->repeat = ( line[strlen("repeat: ")] == '1');
        else if ( g_str_has_prefix(line, "random: ") )
            section->random = ( line[strlen("random: ")] == '1');
        else if ( g_str_has_prefix(line, "single: ") )
            section->single = ( line[strlen("single: ")] == '1');
        else if ( g_str_has_prefix(line, "consume: ") )
            section->consume = ( line[strlen("consume: ")] == '1');
        else if ( g_str_has_prefix(line, "volume: ") )
        {
            gint64 tmp;
            tmp = g_ascii_strtoll(line + strlen("volume: "), NULL, 100);
            section->volume = CLAMP(tmp, -1, 100);
        }
    break;
    case COMMAND_ACTION:
        if ( g_strcmp0(line, "OK") == 0 )
        {
            section->pending = ACTION_NONE;
            _j4status_mpd_section_command(section, COMMAND_QUERY);
        }
    break;
    }

    return TRUE;

fail:
    context->sections = g_list_remove(context->sections, section);
    _j4status_mpd_section_free(section);
    return FALSE;
}

static void
_j4status_mpd_section_start(gpointer data, gpointer user_data)
{
    J4statusMpdSection *section = data;
    if ( section->command == COMMAND_IDLE )
        mpd_async_send_command(section->mpd, "noidle", NULL);
}

static void
_j4status_mpd_section_free(gpointer data)
{
    J4statusMpdSection *section = data;

    j4status_section_free(section->section);

    g_water_mpd_source_unref(section->source);

    g_free(section);
}

static J4statusMpdSection *
_j4status_mpd_section_new(J4statusPluginContext *context, const gchar *host, guint16 port)
{
    J4statusMpdSection *section;
    GError *error = NULL;

    section = g_new0(J4statusMpdSection, 1);
    section->context = context;

    section->source = g_water_mpd_source_new(NULL, host, port, _j4status_mpd_section_line_callback, section, NULL, &error);
    if ( section->source == NULL )
    {
        g_free(section);
        return NULL;
    }
    section->mpd = g_water_mpd_source_get_mpd(section->source);

    section->section = j4status_section_new(context->core);

    j4status_section_set_name(section->section, "mpd");
    j4status_section_set_instance(section->section, host);

    if ( context->config.actions != NULL )
        j4status_section_set_action_callback(section->section, _j4status_mpd_section_action_callback, section);

    if ( ! j4status_section_insert(section->section) )
    {
        _j4status_mpd_section_free(section);
        return NULL;
    }

    section->volume = -1;

    gchar group_name[strlen("MPD ") + strlen(host) + 1];
    g_sprintf(group_name, "MPD %s", host);

    gchar *format = NULL;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file(group_name);
    if ( key_file != NULL )
    {
        format = g_key_file_get_string(key_file, group_name, "Format", NULL);
        g_key_file_free(key_file);
    }

    section->format = j4status_format_string_parse(format, _j4status_mpd_format_tokens, _TOKEN_SIZE, J4STATUS_MPD_DEFAULT_FORMAT, &section->used_tokens);

    if ( section->used_tokens == 0 )
    {
        _j4status_mpd_section_free(section);
        return NULL;
    }

    _j4status_mpd_section_command(section, COMMAND_QUERY);
    return section;
}

static void _j4status_mpd_uninit(J4statusPluginContext *context);

static J4statusPluginContext *
_j4status_mpd_init(J4statusCoreInterface *core)
{
    gchar *host = NULL;
    guint16 port = 0;
    J4statusMpdConfig config = {0};

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("MPD");
    if ( key_file != NULL )
    {
        gint64 tmp;

        host = g_key_file_get_string(key_file, "MPD", "Host", NULL);
        tmp = g_key_file_get_int64(key_file, "MPD", "Port", NULL);
        port = CLAMP(tmp, 0, G_MAXUINT16);

        config.actions = j4status_config_key_file_get_actions(key_file, "MPD", _j4status_mpd_action_list, ACTION_NONE);

        g_key_file_free(key_file);
    }

    if ( host == NULL )
        host = g_strdup(g_getenv("MPD_HOST"));
    if ( port == 0 )
    {
        const gchar *port_env = g_getenv("MPD_PORT");
        if ( port_env != NULL )
        {
            gint64 tmp;
            tmp = g_ascii_strtoll(port_env, NULL, 10);
            port = CLAMP(tmp, 0, G_MAXUINT16);
        }
    }
    if ( host == NULL )
        return NULL;

    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;
    context->config = config;

    {
        J4statusMpdSection *section;
        section = _j4status_mpd_section_new(context, host, port);
        if ( section != NULL )
            context->sections = g_list_prepend(context->sections, section);
    }


    if ( context->sections == NULL )
    {
        _j4status_mpd_uninit(context);
        return NULL;
    }

    return context;
}

static void
_j4status_mpd_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_mpd_section_free);

    if ( context->config.actions != NULL )
        g_hash_table_unref(context->config.actions);

    g_free(context);
}

static void
_j4status_mpd_start(J4statusPluginContext *context)
{
    context->started = TRUE;
    g_list_foreach(context->sections, _j4status_mpd_section_start, context);
}

static void
_j4status_mpd_stop(J4statusPluginContext *context)
{
    context->started = FALSE;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_mpd_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_mpd_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_mpd_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_mpd_stop);
}
