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
#include <libj4status-config.h>

#define TIME_SIZE 4095

typedef struct {
    gboolean show_options;
    gboolean show_volume;
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
} J4statusMpdSectionCommand;

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

    J4statusMpdSectionCommand command;

    gchar *current_song;
    J4statusMpdSectionState state;
    gboolean updating;
    gboolean repeat;
    gboolean random;
    gboolean single;
    gboolean consume;
    gint8 volume;
} J4statusMpdSection;

static void
_j4status_mpd_section_command(J4statusMpdSection *section, J4statusMpdSectionCommand command)
{
    if ( ! section->context->started )
        command = COMMAND_IDLE;

    switch ( command )
    {
    case COMMAND_IDLE:
    {
        const gchar *params[2] = {NULL};
        if ( section->context->config.show_volume )
            params[0] = "mixer";
        if ( section->context->config.show_options )
        {
            params[1] = params[0];
            params[0] = "options";
        }
        mpd_async_send_command(section->mpd, "idle", "player", "update", params[0], params[1], NULL);
    }
    break;
    case COMMAND_QUERY:
        mpd_async_send_command(section->mpd, "command_list_begin", NULL);
        mpd_async_send_command(section->mpd, "status", NULL);
        mpd_async_send_command(section->mpd, "currentsong", NULL);
        mpd_async_send_command(section->mpd, "command_list_end", NULL);
    break;
    }
    section->command = command;
}

static void
_j4status_mpd_section_update(J4statusMpdSection *section)
{
    J4statusState state = J4STATUS_STATE_NO_STATE;
    const gchar *status = "";
    const gchar *song = ( section->current_song != NULL ) ? section->current_song : "No song";
    const gchar *updating = section->updating ? " (Updating database)" : "";
    gchar options[8] = {0};
    gchar volume[6] = {0};

    switch ( section->state )
    {
    case STATE_PLAY:
        state = J4STATUS_STATE_GOOD;
        status = "";
    break;
    case STATE_PAUSE:
        state = J4STATUS_STATE_AVERAGE;
        status = "[Paused] ";
    break;
    case STATE_STOP:
        state = J4STATUS_STATE_BAD;
        status = "[Stopped] ";
    break;
    }

    if ( section->context->config.show_options )
    {
        options[0] = ' ';
        options[1] = '[';
        options[2] = section->repeat ? 'r' : ' ';
        options[3] = section->random ? 'z' : ' ';
        options[4] = section->single ? '1' : ' ';
        options[5] = section->consume ? '-' : ' ';
        options[6] = ']';
    }

    if ( section->volume > 0 )
        g_sprintf(volume, " %hd%%", section->volume);


    gchar *value;
    value = g_strdup_printf("%s%s%s%s%s", status, song, updating, options, volume);
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
            break;
        if ( ! g_str_has_prefix(line, "OK") )
            goto fail;
        if ( g_strcmp0(line, "OK") == 0 )
            _j4status_mpd_section_command(section, COMMAND_QUERY);
        section->updating = FALSE;
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
        {
            g_free(section->current_song);
            section->current_song = g_strdup(line + strlen("Title: "));
        }
        else if ( section->context->config.show_options && g_str_has_prefix(line, "repeat: ") )
            section->repeat = ( line[strlen("repeat: ")] == '1');
        else if ( section->context->config.show_options && g_str_has_prefix(line, "random: ") )
            section->random = ( line[strlen("random: ")] == '1');
        else if ( section->context->config.show_options && g_str_has_prefix(line, "single: ") )
            section->single = ( line[strlen("single: ")] == '1');
        else if ( section->context->config.show_options && g_str_has_prefix(line, "consume: ") )
            section->consume = ( line[strlen("consume: ")] == '1');
        else if ( section->context->config.show_volume && g_str_has_prefix(line, "volume: ") )
        {
            gint64 tmp;
            tmp = g_ascii_strtoll(line + strlen("volume: "), NULL, 100);
            section->volume = CLAMP(tmp, -1, 100);
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

    j4status_section_insert(section->section);

    section->volume = -1;

    _j4status_mpd_section_command(section, COMMAND_QUERY);
    return section;
}

static J4statusPluginContext *
_j4status_mpd_init(J4statusCoreInterface *core)
{
    gchar *host = NULL;
    guint16 port = 0;
    J4statusMpdConfig config = {0};

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("MPD");
    if ( key_file != NULL )
    {
        gint64 tmp;
        host = g_key_file_get_string(key_file, "MPD", "Host", NULL);
        tmp = g_key_file_get_int64(key_file, "MPD", "Port", NULL);
        port = CLAMP(tmp, 0, G_MAXUINT16);

        config.show_options = g_key_file_get_boolean(key_file, "MPD", "ShowOptions", NULL);
        config.show_volume = g_key_file_get_boolean(key_file, "MPD", "ShowVolume", NULL);
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


    if ( context->sections != NULL )
        return context;

    g_free(context);
    return NULL;
}

static void
_j4status_mpd_section_free(gpointer data)
{
    J4statusMpdSection *section = data;

    j4status_section_free(section->section);

    g_water_mpd_source_unref(section->source);

    g_free(section);
}

static void
_j4status_mpd_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_mpd_section_free);

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
