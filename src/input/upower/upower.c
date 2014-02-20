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

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <glib.h>
#include <upower.h>

#include <j4status-plugin-input.h>
#include <libj4status-config.h>

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *sections;
    UpClient *up_client;
    gboolean started;
};

static void
#if UP_CHECK_VERSION(0,99,0)
_j4status_upower_device_changed(GObject *device, GParamSpec *pspec, gpointer user_data)
#else /* ! UP_CHECK_VERSION(0,99,0) */
_j4status_upower_device_changed(GObject *device, gpointer user_data)
#endif /* ! UP_CHECK_VERSION(0,99,0) */
{
    J4statusSection *section = user_data;
    J4statusPluginContext *context = j4status_section_get_user_data(section);

    UpDeviceState device_state;
    gdouble percentage = -1;
    gint64 time = -1;
    const gchar *status = "Bat";
    J4statusState state = J4STATUS_STATE_NO_STATE;

    GValue val = G_VALUE_INIT;

    g_value_init(&val, G_TYPE_DOUBLE);
    g_object_get_property(device, "percentage", &val);
    percentage = g_value_get_double(&val);
    g_value_unset(&val);

    g_value_init(&val, G_TYPE_INT);
    g_object_get_property(device, "state", &val);
    device_state = g_value_get_int(&val);
    g_value_unset(&val);

    switch ( device_state )
    {
    case UP_DEVICE_STATE_LAST: /* Size placeholder */
    case UP_DEVICE_STATE_UNKNOWN:
        j4status_section_set_state(section, J4STATUS_STATE_UNAVAILABLE);
        j4status_section_set_value(section, g_strdup("No battery"));
        return;
    case UP_DEVICE_STATE_EMPTY:
        status = "Empty";
        state = J4STATUS_STATE_BAD | J4STATUS_STATE_URGENT;
    break;
    case UP_DEVICE_STATE_FULLY_CHARGED:
        status = "Full";
        state = J4STATUS_STATE_GOOD;
    break;
    case UP_DEVICE_STATE_CHARGING:
    case UP_DEVICE_STATE_PENDING_CHARGE:
        status = "Chr";
        state = J4STATUS_STATE_AVERAGE;

        g_value_init(&val, G_TYPE_INT64);
        g_object_get_property(device, "time-to-full", &val);
        time = g_value_get_int64(&val);
        g_value_unset(&val);
    break;
    case UP_DEVICE_STATE_DISCHARGING:
    case UP_DEVICE_STATE_PENDING_DISCHARGE:
        status = "Bat";
        if ( percentage < 15 )
            state = J4STATUS_STATE_BAD;
        else
            state = J4STATUS_STATE_AVERAGE;

        if ( percentage < 5 )
            state |= J4STATUS_STATE_URGENT;

        g_value_init(&val, G_TYPE_INT64);
        g_object_get_property(device, "time-to-empty", &val);
        time = g_value_get_int64(&val);
        g_value_unset(&val);
    break;
    }
    j4status_section_set_state(section, state);

    gchar *value;
    if ( percentage < 0 )
        value = g_strdup_printf("%s", status);
    else if ( time < 1 )
        value = g_strdup_printf("%s %.02f%%", status, percentage);
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

        value = g_strdup_printf("%s %.02f%% (%02ju:%02ju:%02jd)", status, percentage, h, m, time);
    }
    j4status_section_set_value(section, value);

    if ( context->started || ( state & J4STATUS_STATE_URGENT ) )
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

    gboolean all_devices = FALSE;

    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("UPower");
    if ( key_file != NULL )
    {
        all_devices = g_key_file_get_boolean(key_file, "UPower", "AllDevices", NULL);
        g_key_file_free(key_file);
    }

    GPtrArray *devices;
    GObject *device;
    guint i;

    devices = up_client_get_devices(context->up_client);
    for ( i = 0 ; i < devices->len ; ++i )
    {
        device = g_ptr_array_index(devices, i);

        GValue value = G_VALUE_INIT;
        UpDeviceKind kind;

        g_value_init(&value, G_TYPE_INT);
        g_object_get_property(device, "kind", &value);
        kind = g_value_get_int(&value);
        g_value_unset(&value);

        const gchar *path;
        const gchar *name = NULL;
        const gchar *instance = NULL;
        const gchar *label = NULL;

        path = up_device_get_object_path(UP_DEVICE(device));
        instance = g_utf8_strrchr(path, -1, '/') + strlen("/") + strlen(up_device_kind_to_string(kind)) + strlen("_");

        switch ( kind )
        {
        case UP_DEVICE_KIND_BATTERY:
            name = "upower-battery";
        break;
        case UP_DEVICE_KIND_UPS:
            if ( ! all_devices )
                continue;
            name = "upower-ups";
            label = "UPS";
        break;
        case UP_DEVICE_KIND_MONITOR:
            if ( ! all_devices )
                continue;
            name = "upower-monitor";
            label = "Monitor";
        break;
        case UP_DEVICE_KIND_MOUSE:
            if ( ! all_devices )
                continue;
            name = "upower-mouse";
            label = "Mouse";
        break;
        case UP_DEVICE_KIND_KEYBOARD:
            if ( ! all_devices )
                continue;
            name = "upower-keyboard";
            label = "Keyboard";
        break;
        case UP_DEVICE_KIND_PDA:
            if ( ! all_devices )
                continue;
            name = "upower-pda";
            label = "PDA";
        break;
        case UP_DEVICE_KIND_PHONE:
            if ( ! all_devices )
                continue;
            name = "upower-phone";
            label = "Phone";
        break;
        case UP_DEVICE_KIND_MEDIA_PLAYER:
            if ( ! all_devices )
                continue;
            name = "upower-media-player";
            label = "Media player";
        break;
        case UP_DEVICE_KIND_TABLET:
            if ( ! all_devices )
                continue;
            name = "upower-tablet";
            label = "Tablet";
        break;
        case UP_DEVICE_KIND_COMPUTER:
            if ( ! all_devices )
                continue;
            name = "upower-computer";
            label = "Computer";
        break;
        case UP_DEVICE_KIND_UNKNOWN:
        case UP_DEVICE_KIND_LINE_POWER:
        case UP_DEVICE_KIND_LAST: /* Size placeholder */
            continue;
        }

        J4statusSection *section;
        section = j4status_section_new(context->core, name, instance, context);
        if ( label != NULL )
            j4status_section_set_label(section, label);

#if UP_CHECK_VERSION(0,99,0)
        g_signal_connect(device, "notify", G_CALLBACK(_j4status_upower_device_changed), section);
        _j4status_upower_device_changed(device, NULL, section);
#else /* ! UP_CHECK_VERSION(0,99,0) */
        g_signal_connect(device, "changed", G_CALLBACK(_j4status_upower_device_changed), section);
        _j4status_upower_device_changed(device, section);
#endif /* ! UP_CHECK_VERSION(0,99,0) */

        context->sections = g_list_prepend(context->sections, section);

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

static void
_j4status_upower_start(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = TRUE;
}

static void
_j4status_upower_stop(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = FALSE;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_upower_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_upower_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_upower_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_upower_stop);
}
