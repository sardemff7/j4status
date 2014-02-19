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
#include <upower.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *sections;
    UpClient *up_client;
};

static void
#if UP_CHECK_VERSION(0,99,0)
_j4status_upower_battery_changed(UpDevice *device, GParamSpec *pspec, gpointer user_data)
#else /* ! UP_CHECK_VERSION(0,99,0) */
_j4status_upower_battery_changed(UpDevice *device, gpointer user_data)
#endif /* ! UP_CHECK_VERSION(0,99,0) */
{
    J4statusSection *section = user_data;
    J4statusPluginContext *context = j4status_section_get_user_data(section);

    gdouble percentage = -1;
    gint64 time = -1;
    const gchar *state = "Bat";

    j4status_section_set_state(section, J4STATUS_STATE_NO_STATE);

    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_INT);
    g_object_get_property(G_OBJECT(device), "state", &value);
    switch ( g_value_get_int(&value) )
    {
    case UP_DEVICE_STATE_UNKNOWN:
        j4status_section_set_state(section, J4STATUS_STATE_UNAVAILABLE);
        j4status_section_set_value(section, g_strdup("No battery"));
        return;
    case UP_DEVICE_STATE_EMPTY:
        state = "Empty";
    break;
    case UP_DEVICE_STATE_FULLY_CHARGED:
        state = "Full";
    break;
    case UP_DEVICE_STATE_CHARGING:
    case UP_DEVICE_STATE_PENDING_CHARGE:
        state = "Chr";
    break;
    case UP_DEVICE_STATE_DISCHARGING:
    case UP_DEVICE_STATE_PENDING_DISCHARGE:
        state = "Bat";
    break;
    }
    g_value_unset(&value);

    g_value_init(&value, G_TYPE_DOUBLE);
    g_object_get_property(G_OBJECT(device), "percentage", &value);
    percentage = g_value_get_double(&value);
    g_value_unset(&value);

    switch ( state[0] )
    {
    case 'B':
        if ( percentage < 5 )
            j4status_section_set_state(section, J4STATUS_STATE_URGENT);
        else if ( percentage < 15 )
            j4status_section_set_state(section, J4STATUS_STATE_BAD);
        else
            j4status_section_set_state(section, J4STATUS_STATE_AVERAGE);

        g_value_init(&value, G_TYPE_INT64);
        g_object_get_property(G_OBJECT(device), "time-to-empty", &value);
        time = g_value_get_int64(&value);
        g_value_unset(&value);
    break;
    case 'E':
        j4status_section_set_state(section, J4STATUS_STATE_URGENT);
    break;
    case 'C':
        j4status_section_set_state(section, J4STATUS_STATE_AVERAGE);

        g_value_init(&value, G_TYPE_INT64);
        g_object_get_property(G_OBJECT(device), "time-to-full", &value);
        time = g_value_get_int64(&value);
        g_value_unset(&value);
    break;
    case 'F':
        j4status_section_set_state(section, J4STATUS_STATE_GOOD);
    break;
    }

    if ( percentage < 0 )
        j4status_section_set_value(section, g_strdup_printf("%s", state));
    else if ( time < 1 )
        j4status_section_set_value(section, g_strdup_printf("%s %.02f%%", state, percentage));
    else
    {
        guint64 h = 0;
        guint64 m = 0;

        if ( time > 3600 )
        {
            h = time / 3600;
            time %= 3600;
        }
        if ( time > 60 )
        {
            m = time / 60;
            time %= 60;
        }

        j4status_section_set_value(section, g_strdup_printf("%s %.02f%% (%02ju:%02ju:%02jd)", state, percentage, h, m, time));
    }

    libj4status_core_trigger_display(context->core);
}

static J4statusPluginContext *
_j4status_upower_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;
    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->up_client = up_client_new();

#if ! UP_CHECK_VERSION(0,99,0)
    if ( ! up_client_enumerate_devices_sync(context->up_client, NULL, NULL) )
    {
        g_object_unref(context->up_client);
        return NULL;
    }
#endif /* ! UP_CHECK_VERSION(0,99,0) */

    /* Not using the configuration file
    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("Battery");
    if ( key_file != NULL )
    {
        g_key_file_free(key_file);
    }
    */

    GPtrArray *devices;
    UpDevice *device;
    gchar *name;
    guint i;

    devices = up_client_get_devices(context->up_client);
    for ( i = 0 ; i < devices->len ; ++i )
    {
        device = g_ptr_array_index(devices, i);
        name = g_path_get_basename(up_device_get_object_path(device));
        J4statusSection *section;

        section = j4status_section_new("upower", name, context);

        GValue value = G_VALUE_INIT;

        g_value_init(&value, G_TYPE_INT);
        g_object_get_property(G_OBJECT(device), "kind", &value);
        switch ( g_value_get_int(&value) )
        {
        case UP_DEVICE_KIND_BATTERY:
#if UP_CHECK_VERSION(0,99,0)
            g_signal_connect(device, "notify", G_CALLBACK(_j4status_upower_battery_changed), section);
            _j4status_upower_battery_changed(device, NULL, section);
#else /* ! UP_CHECK_VERSION(0,99,0) */
            g_signal_connect(device, "changed", G_CALLBACK(_j4status_upower_battery_changed), section);
            _j4status_upower_battery_changed(device, section);
#endif /* ! UP_CHECK_VERSION(0,99,0) */
        break;
        default:
            continue;
        }

        context->sections = g_list_prepend(context->sections, section);

        g_free(name);
    }
    g_ptr_array_unref(devices);

    context->sections = g_list_reverse(context->sections);
    return context;
}

static void
_j4status_upower_uninit(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    g_list_free_full(context->sections, (GDestroyNotify) j4status_section_free);

    g_free(context);
}

static GList **
_j4status_upower_get_sections(J4statusPluginContext *context)
{
    if ( context == NULL )
        return NULL;

    return &context->sections;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_upower_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_upower_uninit);

    libj4status_input_plugin_interface_add_get_sections_callback(interface, _j4status_upower_get_sections);
}
