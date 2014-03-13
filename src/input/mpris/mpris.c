/*
 * j4status - Status line generator
 *
 * Copyright © 2012-2013 Quentin "Sardem FF7" Glidic
 *
 * Mpris plugin by Tony Crisci <tony@dubstepdish.com>
 *
 * MPRIS D-Bus Interface Specification:
 * http://specifications.freedesktop.org/mpris-spec/latest/index.html
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

#include <glib-object.h>

#include <j4status-plugin-input.h>
#include <libj4status-config.h>

#include "mpris-generated.h"

static const gchar *MPRIS_DEFAULT_PLAYER = "vlc";

struct _J4statusPluginContext {
    OrgMprisMediaPlayer2Player *proxy;
    J4statusSection *section;
};

static gchar *
_j4status_mpris_build_status_text(GVariant *metadata)
{
    /* MPRIS v2 metadata guidelines:
     * http://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata */
    gchar *status_text;
    gchar *title = NULL;
    GVariant *artist_list = NULL;
    gchar **artist_strv = NULL;
    gchar *artist = NULL;

    /* return empty string unless the player is active */
    if ( metadata == NULL || !g_variant_check_format_string(metadata, "a{sv}", FALSE) )
        return g_strdup("");

    g_variant_lookup(metadata, "xesam:title", "s", &title);
    artist_list = g_variant_lookup_value(metadata, "xesam:artist", G_VARIANT_TYPE_ARRAY);

    if ( artist_list != NULL)
    {
        artist_strv = g_variant_dup_strv(artist_list, NULL);
        artist = g_strjoinv(", ", artist_strv);
    }

    status_text = ( artist == NULL
                  ? g_strdup(title)
                  : g_strjoin(" - ", artist, title, NULL) );

    if ( artist_list != NULL )
        g_variant_unref(artist_list);
    g_free(title);
    g_free(artist);
    g_strfreev(artist_strv);

    return status_text;
}

static void
_j4status_properties_changed_callback(GDBusProxy *_proxy, GVariant *changed_properties, const gchar *const *invalidated_properties, gpointer user_data)
{
    OrgMprisMediaPlayer2Player *proxy;
    GVariant *metadata;
    gchar *status_text;

    J4statusSection *section = user_data;

    proxy = ORG_MPRIS_MEDIA_PLAYER2_PLAYER(_proxy);
    metadata = org_mpris_media_player2_player_get_metadata(proxy);

    status_text = _j4status_mpris_build_status_text(metadata);

    j4status_section_set_value(section, status_text);
}

static void _j4status_mpris_uninit(J4statusPluginContext *context);

static J4statusPluginContext *
_j4status_mpris_init(J4statusCoreInterface *core)
{
    GError *error = NULL;
    GKeyFile *key_file;
    OrgMprisMediaPlayer2Player *proxy;
    gchar *player_name;
    gchar *bus_name;

    key_file = libj4status_config_get_key_file("Mpris");

    player_name = ( key_file == NULL || !g_key_file_has_key(key_file, "Mpris", "Player", NULL)
                  ? g_strdup(MPRIS_DEFAULT_PLAYER)
                  : g_key_file_get_string(key_file, "Mpris", "Player", NULL) );

    bus_name = g_strjoin(".", "org.mpris.MediaPlayer2", player_name, NULL);

    proxy = org_mpris_media_player2_player_proxy_new_for_bus_sync(
            G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_NONE,
            bus_name,
            "/org/mpris/MediaPlayer2",
            NULL,
            &error);

    g_free(bus_name);
    g_free(player_name);

    if ( proxy == NULL )
    {
        g_warning("Couldn't establish proxy connection to player: %s", error->message);
        g_clear_error(&error);
        return NULL;
    }

    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->proxy = proxy;

    context->section = j4status_section_new(core);
    j4status_section_set_name(context->section, "mpris");
    if ( ! j4status_section_insert(context->section) )
    {
        _j4status_mpris_uninit(context);
        return NULL;
    }

    g_signal_connect(context->proxy, "g-properties-changed", G_CALLBACK(_j4status_properties_changed_callback), context->section);

    return context;
}

static void
_j4status_mpris_uninit(J4statusPluginContext *context)
{
    j4status_section_free(context->section);

    g_object_unref(context->proxy);

    g_free(context);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_mpris_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_mpris_uninit);
}
