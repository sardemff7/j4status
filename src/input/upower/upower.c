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
#include <upower.h>

#include <j4status-plugin.h>
#include <libj4status-config.h>

struct _J4statusPluginContext {
    UpClient *up_client;
    GList *sections;
};

static void
_j4status_upower_battery_changed(UpDevice *device, gpointer user_data)
{
    J4statusSection *section = user_data;

    gdouble percentage = -1;
    gint64 time = -1;
    const gchar *state = "Bat";

    section->state = J4STATUS_STATE_NO_STATE;

    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_INT);
    g_object_get_property(G_OBJECT(device), "state", &value);
    switch ( g_value_get_int(&value) )
    {
    case UP_DEVICE_STATE_UNKNOWN:
        section->state = J4STATUS_STATE_UNAVAILABLE;
        section->value = g_strdup("No battery");
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
            section->state = J4STATUS_STATE_URGENT;
        else if ( percentage < 15 )
            section->state = J4STATUS_STATE_BAD;
        else
            section->state = J4STATUS_STATE_AVERAGE;

        g_value_init(&value, G_TYPE_INT64);
        g_object_get_property(G_OBJECT(device), "time-to-empty", &value);
        time = g_value_get_int64(&value);
        g_value_unset(&value);
    break;
    case 'E':
        section->state = J4STATUS_STATE_URGENT;
    break;
    case 'C':
        section->state = J4STATUS_STATE_AVERAGE;

        g_value_init(&value, G_TYPE_INT64);
        g_object_get_property(G_OBJECT(device), "time-to-full", &value);
        time = g_value_get_int64(&value);
        g_value_unset(&value);
    break;
    case 'F':
        section->state = J4STATUS_STATE_GOOD;
    break;
    }

    if ( percentage < 0 )
        section->value = g_strdup_printf("%s", state);
    else if ( time < 1 )
        section->value = g_strdup_printf("%s %.02f%%", state, percentage);
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

        section->value = g_strdup_printf("%s %.02f%% (%02ju:%02ju:%02jd)", state, percentage, h, m, time);
    }
    section->dirty = TRUE;
}

static J4statusPluginContext *
_j4status_upower_init()
{
    J4statusPluginContext *context;
    context = g_new0(J4statusPluginContext, 1);

    context->up_client = up_client_new();

    if ( ! up_client_enumerate_devices_sync(context->up_client, NULL, NULL) )
    {
        g_object_unref(context->up_client);
        return NULL;
    }

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("Battery");
    if ( key_file != NULL )
    {
        g_key_file_unref(key_file);
    }

    GPtrArray *devices;
    UpDevice *device;
    guint i;

    devices = up_client_get_devices(context->up_client);
    for ( i = 0 ; i < devices->len ; ++i )
    {
        device = g_ptr_array_index(devices, i);
        J4statusSection *section;

        section = g_new0(J4statusSection, 1);

        GValue value = G_VALUE_INIT;

        g_value_init(&value, G_TYPE_INT);
        g_object_get_property(G_OBJECT(device), "kind", &value);
        switch ( g_value_get_int(&value) )
        {
        case UP_DEVICE_KIND_BATTERY:
            section->name = "battery";
            section->instance = g_strdup_printf("%u", i);
            g_signal_connect(device, "changed", G_CALLBACK(_j4status_upower_battery_changed), section);
            _j4status_upower_battery_changed(device, section);
        break;
        default:
            g_free(section);
            continue;
        }

        context->sections = g_list_prepend(context->sections, section);
    }
    g_ptr_array_unref(devices);

    context->sections = g_list_reverse(context->sections);
    return context;
}

static void
_j4status_upower_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, g_free);

    g_free(context);
}

static GList **
_j4status_upower_get_sections(J4statusPluginContext *context)
{
    return &context->sections;
}

void
j4status_input_plugin(J4statusInputPlugin *plugin)
{
    plugin->init   = _j4status_upower_init;
    plugin->uninit = _j4status_upower_uninit;

    plugin->get_sections = _j4status_upower_get_sections;
}
