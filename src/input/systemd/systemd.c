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

#include <glib.h>
#include <gio/gio.h>

#include <j4status-plugin-input.h>
#include <libj4status-config.h>

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_INTERFACE_NAME SYSTEMD_BUS_NAME ".Manager"
#define SYSTEMD_UNIT_INTERFACE_NAME SYSTEMD_BUS_NAME ".Unit"

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *sections;
    GDBusConnection *connection;
    gboolean started;
    GDBusProxy *manager;
};

typedef struct {
    J4statusPluginContext *context;
    J4statusSection *section;
    gchar *unit_name;
    GDBusProxy *unit;
} J4statusSystemdSection;

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
    GVariant *ret;

    ret = g_dbus_proxy_call_sync(context->manager, "GetUnit", g_variant_new("(s)", unit_name), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if ( ret == NULL )
        return NULL;

    const gchar *unit_object_path;
    g_variant_get(ret, "(&o)", &unit_object_path);

    GError *error = NULL;
    GDBusProxy *unit;
    unit = g_dbus_proxy_new_sync(context->connection, G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES, NULL, SYSTEMD_BUS_NAME, unit_object_path, SYSTEMD_UNIT_INTERFACE_NAME, NULL, &error);
    if ( unit == NULL )
        g_warning("Could not monitor unit %s: %s", unit_name, error->message);
    g_clear_error(&error);

    g_variant_unref(ret);

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
_j4status_systemd_unit_state_changed(GDBusProxy *gobject, GVariant *changed_properties, GStrv invalidated_properties, gpointer user_data)
{
    J4statusSystemdSection *section = user_data;

    J4statusState state;
    gchar *status;
    status = _j4status_systemd_dbus_get_property_string(section->unit, "ActiveState", "Unknown");
    if ( ( g_strcmp0(status, "active") == 0 ) || ( g_strcmp0(status, "reloading") == 0 ) )
        state = J4STATUS_STATE_GOOD;
    else if ( ( g_strcmp0(status, "failed") == 0 ) )
        state = J4STATUS_STATE_BAD;
    else
        state = J4STATUS_STATE_NO_STATE;

    j4status_section_set_state(section->section, state);
    j4status_section_set_value(section->section, status);
}

static void
_j4status_systemd_section_attach_unit(gpointer data, gpointer user_data)
{
    J4statusPluginContext *context = user_data;
    J4statusSystemdSection *section = data;

    if ( section->unit != NULL )
        return;

    section->unit = _j4status_systemd_dbus_get_unit(context, section->unit_name);

    if ( section->unit != NULL )
    {
        g_signal_connect(section->unit, "g-properties-changed", G_CALLBACK(_j4status_systemd_unit_state_changed), section);
        _j4status_systemd_unit_state_changed(section->unit, NULL, NULL, section);
    }
}

static void
_j4status_systemd_section_detach_unit(gpointer data, gpointer user_data)
{
    J4statusSystemdSection *section = data;

    if ( section->unit != NULL )
        g_object_unref(section->unit);
    section->unit = NULL;
    j4status_section_set_state(section->section, J4STATUS_STATE_UNAVAILABLE);
    j4status_section_set_value(section->section, NULL);
}

static void
_j4status_systemd_bus_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, gpointer user_data)
{
    J4statusPluginContext *context = user_data;

    if ( g_strcmp0(signal_name, "Reloading") == 0 )
    {
        gboolean reloading;
        g_variant_get(parameters, "(b)", &reloading);
        if ( reloading )
            g_list_foreach(context->sections, _j4status_systemd_section_detach_unit, context);
        else
            g_list_foreach(context->sections, _j4status_systemd_section_attach_unit, context);
    }
}


static void
_j4status_systemd_section_new(J4statusPluginContext *context, gchar *unit_name)
{
    J4statusSystemdSection *section;

    section = g_new0(J4statusSystemdSection, 1);
    section->context = context;
    section->unit_name = unit_name;
    section->section = j4status_section_new(context->core);

    j4status_section_set_name(section->section, "systemd");
    j4status_section_set_instance(section->section, unit_name);
    j4status_section_set_label(section->section, unit_name);
    /* Possible unit states:
     * loaded failed active inactive not-found listening running waiting plugged mounted exited dead masked */
    j4status_section_set_max_width(section->section, -strlen("listening"));

    j4status_section_insert(section->section);
    context->sections = g_list_prepend(context->sections, section);
}

static void
_j4status_systemd_section_free(gpointer data)
{
    J4statusSystemdSection *section = data;

    if ( section->unit != NULL )
        g_object_unref(section->unit);

    j4status_section_free(section->section);
    g_free(section->unit_name);

    g_free(section);
}

J4statusPluginContext *
_j4status_systemd_init(J4statusCoreInterface *core)
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
    g_key_file_free(key_file);

    GError *error = NULL;

    GDBusConnection *connection = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if ( connection == NULL )
    {
        g_warning("Couldn't connect to D-Bus: %s", error->message);
        goto fail;
    }

    GDBusProxy *manager;
    manager = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL, SYSTEMD_BUS_NAME, SYSTEMD_OBJECT_PATH, SYSTEMD_MANAGER_INTERFACE_NAME, NULL, &error);
    if ( manager == NULL )
    {
        g_warning("Couldn't connect to systemd manager D-Bus interface: %s", error->message);
        goto fail;
    }

    J4statusPluginContext *context = NULL;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->connection = connection;
    context->manager = manager;

    gchar **unit;
    for ( unit = units ; *unit != NULL ; ++unit )
        _j4status_systemd_section_new(context, *unit);
    g_free(units);

    g_signal_connect(context->manager, "g-signal", G_CALLBACK(_j4status_systemd_bus_signal), context);

    return context;

fail:
    g_clear_error(&error);
    if ( connection != NULL )
        g_object_unref(connection);
    g_strfreev(units);

    return NULL;
}

static void
_j4status_systemd_uninit(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    g_list_free_full(context->sections, _j4status_systemd_section_free);

    g_object_unref(context->manager);
    g_object_unref(context->connection);

    g_free(context);
}

static void
_j4status_systemd_start(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = TRUE;
    _j4status_systemd_dbus_call(context->manager, "Subscribe", NULL);
    g_list_foreach(context->sections, _j4status_systemd_section_attach_unit, context);
}

static void
_j4status_systemd_stop(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = FALSE;
    _j4status_systemd_dbus_call(context->manager, "Unsubscribe", NULL);
    g_list_foreach(context->sections, _j4status_systemd_section_detach_unit, context);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_systemd_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_systemd_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_systemd_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_systemd_stop);
}
