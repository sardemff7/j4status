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

#include <glib.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

#include "dbus.h"

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"

struct _J4statusPluginContext {
    J4statusCoreContext *core;
    J4statusCoreInterface *core_interface;
    GList *sections;
    gchar **units;
    GRegex *dbus_name_regex;
    guint watch_id;
    GDBusConnection *connection;
    gboolean started;
    J4statusSystemdManager *manager;
};

typedef struct {
    J4statusPluginContext *context;
    J4statusSystemdUnit *unit;
} J4statusSystemdSectionContext;


static void
_j4status_systemd_section_free(gpointer data)
{
    J4statusSection *section = data;
    J4statusSystemdSectionContext *section_context = section->user_data;

    if ( section_context->unit != NULL )
        g_object_unref(section_context->unit);

    g_free(section_context);

    g_free(section->value);
    g_free(section->instance);

    g_free(section);
}

static void
_j4status_systemd_unit_state_changed(J4statusSystemdUnit *gobject, GParamSpec *pspec, gpointer user_data)
{
    J4statusSection *section = user_data;
    J4statusSystemdSectionContext *section_context = section->user_data;

    section->value = j4status_systemd_unit_dup_active_state(section_context->unit);
    if ( g_str_equal(section->value, "active") || g_str_equal(section->value, "reloading") )
        section->state = J4STATUS_STATE_GOOD;
    else if ( g_str_equal(section->value, "failed") )
        section->state = J4STATUS_STATE_BAD;
    else
        section->state = J4STATUS_STATE_NO_STATE;

    section->dirty = TRUE;

    libj4status_core_trigger_display(section_context->context->core, section_context->context->core_interface);
}

static void
_j4status_systemd_add_unit(J4statusPluginContext *context, const gchar *unit_name)
{
    GError *error = NULL;
    gchar *unit_object_path;

    if ( ! j4status_systemd_manager_call_get_unit_sync(context->manager, unit_name, &unit_object_path, NULL, &error) )
    {
        g_warning("Could get unit object path %s: %s", unit_name, error->message);
        return;
    }

    J4statusSystemdUnit *unit;
    unit = j4status_systemd_unit_proxy_new_sync(context->connection, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES, SYSTEMD_BUS_NAME, unit_object_path, NULL, &error);
    g_free(unit_object_path);
    if ( unit == NULL )
    {
        g_warning("Could not monitor unit %s: %s", unit_name, error->message);
        return;
    }

    J4statusSystemdSectionContext *section_context;
    J4statusSection *section;

    section_context = g_new0(J4statusSystemdSectionContext, 1);
    section_context->context = context;
    section_context->unit = unit;

    section = g_new0(J4statusSection, 1);
    section->name = "systemd";
    section->instance = g_strdup(unit_name);
    section->label = section->instance;
    section->user_data = section_context;

    g_signal_connect(unit, "notify::active-state", G_CALLBACK(_j4status_systemd_unit_state_changed), section);
    _j4status_systemd_unit_state_changed(unit, NULL, section);

    context->sections = g_list_prepend(context->sections, section);
}

static void
_j4status_systemd_bus_appeared(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data)
{
    GError *error = NULL;
    J4statusPluginContext *context = user_data;

    context->connection = connection;

    context->manager = j4status_systemd_manager_proxy_new_sync(context->connection, G_DBUS_PROXY_FLAGS_NONE, SYSTEMD_BUS_NAME, SYSTEMD_OBJECT_PATH, NULL, &error);
    if ( context->manager == NULL )
    {
        g_warning("Could not connect to systemd manager: %s", error->message);
        g_clear_error(&error);
        return;
    }
    if ( context->started )
    {
        if ( ! j4status_systemd_manager_call_subscribe_sync(context->manager, NULL, &error) )
        {
            g_warning("Could not subscribe to systemd events: %s", error->message);
            g_clear_error(&error);
        }
    }

    gchar **unit;
    for ( unit = context->units ; *unit != NULL ; ++unit )
        _j4status_systemd_add_unit(context, *unit);
    context->sections = g_list_reverse(context->sections);
}

static void
_j4status_systemd_bus_vanished(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    J4statusPluginContext *context = user_data;

    context->connection = connection;

    g_object_unref(context->manager);
    context->manager = NULL;

    g_list_free_full(context->sections, _j4status_systemd_section_free);
    context->sections = NULL;
}

J4statusPluginContext *
_j4status_systemd_init(J4statusCoreContext *core, J4statusCoreInterface *core_interface)
{
    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("systemd");
    if ( key_file == NULL )
        return NULL;

    gchar **units;
    units = g_key_file_get_string_list(key_file, "systemd", "Units", NULL, NULL);
    if ( units == NULL )
    {
        g_key_file_free(key_file);
        return NULL;
    }

    J4statusPluginContext *context = NULL;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;
    context->core_interface = core_interface;

    context->dbus_name_regex = g_regex_new("[-._@:\\\\]", G_REGEX_OPTIMIZE, 0, NULL);

    context->units = units;
    context->watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, SYSTEMD_BUS_NAME, G_BUS_NAME_WATCHER_FLAGS_NONE, _j4status_systemd_bus_appeared, _j4status_systemd_bus_vanished, context, NULL);

    return context;
}

static void
_j4status_systemd_uninit(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    g_list_free_full(context->sections, _j4status_systemd_section_free);

    g_bus_unwatch_name(context->watch_id);

    g_regex_unref(context->dbus_name_regex);

    g_strfreev(context->units);

    g_free(context);
}

static GList **
_j4status_systemd_get_sections(J4statusPluginContext *context)
{
    if ( context == NULL )
        return NULL;

    return &context->sections;
}

static void
_j4status_systemd_start(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = TRUE;
    if ( context->manager != NULL )
        j4status_systemd_manager_call_subscribe_sync(context->manager, NULL, NULL);
}

static void
_j4status_systemd_stop(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = FALSE;
    if ( context->manager != NULL )
        j4status_systemd_manager_call_unsubscribe_sync(context->manager, NULL, NULL);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_systemd_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_systemd_uninit);

    libj4status_input_plugin_interface_add_get_sections_callback(interface, _j4status_systemd_get_sections);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_systemd_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_systemd_stop);
}
