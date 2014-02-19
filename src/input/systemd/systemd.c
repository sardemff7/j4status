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
#include <gio/gio.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_INTERFACE_NAME SYSTEMD_BUS_NAME ".Manager"
#define SYSTEMD_UNIT_INTERFACE_NAME SYSTEMD_BUS_NAME ".Unit"

struct _J4statusPluginContext {
    J4statusCoreContext *core;
    J4statusCoreInterface *core_interface;
    GList *sections;
    gchar **units;
    GRegex *dbus_name_regex;
    guint watch_id;
    GDBusConnection *connection;
    gboolean started;
    GDBusProxy *manager;
};

typedef struct {
    J4statusPluginContext *context;
    GDBusProxy *unit;
} J4statusSystemdSectionContext;

static gboolean
_j4status_systemd_dbus_call(GDBusProxy *proxy, const gchar *method, GError **error)
{
    GVariant *ret;
    ret = g_dbus_proxy_call_sync(proxy, method, g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, error);
    if ( ret == NULL )
        return FALSE;
    g_variant_get(ret, "()");
    g_variant_unref(ret);
    return TRUE;
}

static GDBusProxy *
_j4status_systemd_dbus_get_unit(J4statusPluginContext *context, const gchar *unit_name)
{
    GError *error = NULL;
    GVariant *ret;

    ret = g_dbus_proxy_call_sync(context->manager, "GetUnit", g_variant_new("(s)", unit_name), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if ( ret == NULL )
    {
        g_warning("Could get unit object path %s: %s", unit_name, error->message);
        g_clear_error(&error);
        return NULL;
    }

    gchar *unit_object_path;
    g_variant_get(ret, "(o)", &unit_object_path);
    g_variant_unref(ret);

    GDBusProxy *unit;
    unit = g_dbus_proxy_new_sync(context->connection, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES, NULL, SYSTEMD_BUS_NAME, unit_object_path, SYSTEMD_UNIT_INTERFACE_NAME, NULL, &error);
    g_free(unit_object_path);
    if ( unit == NULL )
        g_warning("Could not monitor unit %s: %s", unit_name, error->message);
    g_clear_error(&error);

    return unit;
}

static GVariant *
_j4status_systemd_dbus_get_property(GDBusProxy *proxy, const gchar *property)
{
    GError *error = NULL;
    GVariant *ret, *val = NULL;

    val = g_dbus_proxy_get_cached_property(proxy, property);
    if ( val == NULL )
    {
        ret = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.Properties.Get", g_variant_new("(ss)", g_dbus_proxy_get_interface_name(proxy), property), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        if ( ret == NULL )
            g_warning("Could get property %s . %s: %s", g_dbus_proxy_get_interface_name(proxy), property, error->message);
        else
        {
            g_variant_get(ret, "(v)", &val);
            g_variant_unref(ret);
        }
        g_clear_error(&error);
    }

    return val;
}

static gchar *
_j4status_systemd_dbus_get_property_string(GDBusProxy *proxy, const gchar *property, const gchar *default_value)
{
    GVariant *ret;

    ret = _j4status_systemd_dbus_get_property(proxy, property);
    if ( ret == NULL )
        return g_strdup(default_value);

    gchar *val;
    g_variant_get(ret, "s", &val);
    g_variant_unref(ret);

    return val;
}

static void
_j4status_systemd_section_free(gpointer data)
{
    J4statusSection *section = data;
    J4statusSystemdSectionContext *section_context = j4status_section_get_user_data(section);

    if ( section_context->unit != NULL )
        g_object_unref(section_context->unit);

    g_free(section_context);

    j4status_section_free(section);
}

static void
_j4status_systemd_unit_state_changed(GDBusProxy *gobject, GVariant *changed_properties, GStrv invalidated_properties, gpointer user_data)
{
    J4statusSection *section = user_data;
    J4statusSystemdSectionContext *section_context = j4status_section_get_user_data(section);

    gchar *state;
    state = _j4status_systemd_dbus_get_property_string(section_context->unit, "ActiveState", "Unknown");
    j4status_section_set_value(section, state);
    if ( ( g_strcmp0(state, "active") == 0 ) || ( g_strcmp0(state, "reloading") == 0 ) )
        j4status_section_set_state(section, J4STATUS_STATE_GOOD);
    else if ( ( g_strcmp0(state, "failed") == 0 ) )
        j4status_section_set_state(section, J4STATUS_STATE_BAD);
    else
        j4status_section_set_state(section, J4STATUS_STATE_NO_STATE);

    libj4status_core_trigger_display(section_context->context->core, section_context->context->core_interface);
}

static void
_j4status_systemd_add_unit(J4statusPluginContext *context, const gchar *unit_name)
{
    GDBusProxy *unit;
    unit = _j4status_systemd_dbus_get_unit(context, unit_name);
    if ( unit == NULL )
        return;

    J4statusSystemdSectionContext *section_context;
    J4statusSection *section;

    section_context = g_new0(J4statusSystemdSectionContext, 1);
    section_context->context = context;
    section_context->unit = unit;

    section = j4status_section_new("systemd", unit_name, section_context);
    j4status_section_set_label(section, unit_name);

    g_signal_connect(unit, "g-properties-changed", G_CALLBACK(_j4status_systemd_unit_state_changed), section);
    _j4status_systemd_unit_state_changed(unit, NULL, NULL, section);

    context->sections = g_list_prepend(context->sections, section);
}

static void
_j4status_systemd_bus_appeared(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data)
{
    GError *error = NULL;
    J4statusPluginContext *context = user_data;

    context->connection = connection;

    context->manager = g_dbus_proxy_new_sync(context->connection, G_DBUS_PROXY_FLAGS_NONE, NULL, SYSTEMD_BUS_NAME, SYSTEMD_OBJECT_PATH, SYSTEMD_MANAGER_INTERFACE_NAME, NULL, &error);
    if ( context->manager == NULL )
    {
        g_warning("Could not connect to systemd manager: %s", error->message);
        g_clear_error(&error);
        return;
    }
    if ( context->started )
    {
        if ( ! _j4status_systemd_dbus_call(context->manager, "Subscribe", &error) )
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
        _j4status_systemd_dbus_call(context->manager, "Subscribe", NULL);
}

static void
_j4status_systemd_stop(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = FALSE;
    if ( context->manager != NULL )
        _j4status_systemd_dbus_call(context->manager, "Unsubscribe", NULL);
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
