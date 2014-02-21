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

#include <netdb.h>
#include <glib.h>
#include <libnm-glib/nm-client.h>
#include <libnm-glib/nm-device.h>
#include <libnm-glib/nm-device-ethernet.h>
#include <libnm-glib/nm-device-wifi.h>

#include <j4status-plugin-input.h>
#include <libj4status-config.h>

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GHashTable *sections;

    /* Generic configuration */
    gboolean show_unknown;
    gboolean show_unmanaged;
    gboolean hide_unavailable;

    /* WiFi devices configuration */
    struct {
        gboolean show_available_aps_number;
    } wifi;

    NMClient *nm_client;
    gboolean started;
};

typedef struct {
    J4statusPluginContext *context;
    const gchar *interface;
    NMDevice *device;
    J4statusSection *section;

    gulong state_changed_handler;

    /* For WiFi devices */
    NMAccessPoint *ap;

    gulong bitrate_handler;
    gulong active_access_point_handler;
    gulong strength_handler;
} J4statusNmSection;

static void
_j4status_nm_device_get_addresses_ipv4(J4statusPluginContext *context, NMDevice *device, GString *addresses)
{
    NMIP4Config *ip4_config;

    ip4_config = nm_device_get_ip4_config(device);
    if ( ip4_config == NULL )
        return;

    const GSList *address_;
    for ( address_ = nm_ip4_config_get_addresses(ip4_config) ; address_ != NULL ; address_ = g_slist_next(address_) )
    {
        guint32 address;
        address = nm_ip4_address_get_address(address_->data);
        g_string_append_printf(addresses, "%u.%u.%u.%u", (address >> 0) & 255, (address >> 8) & 255, (address >> 16) & 255, (address >> 24) & 255);
        if (g_slist_next(address_) != NULL )
            g_string_append(addresses, ", ");
    }
    g_string_append(addresses, " ");
}

static void
_j4status_nm_device_get_addresses_ipv6(J4statusPluginContext *context, NMDevice *device, GString *addresses)
{
    NMIP6Config *ip6_config;

    ip6_config = nm_device_get_ip6_config(device);
    if ( ip6_config == NULL )
        return;

    const GSList *address_;
    for ( address_ = nm_ip6_config_get_addresses(ip6_config) ; address_ != NULL ; address_ = g_slist_next(address_) )
    {
        const struct in6_addr *address;
        address = nm_ip6_address_get_address(address_->data);

        guint b = 0;
        gboolean shortened = FALSE;
        gboolean was_shortened = FALSE;
        guint fragment;
        for (;;)
        {
            fragment = ( address->s6_addr[b] << 8 ) + address->s6_addr[b+1];
            if ( ( fragment == 0 ) && ( ! was_shortened ) )
                shortened = TRUE;
            else
            {
                if ( shortened )
                {
                    g_string_append_c(addresses, ':');
                    shortened = FALSE;
                    was_shortened = TRUE;
                }
                g_string_append_printf(addresses, "%x", fragment);
            }
            b += 2;
            if ( b >= 16 )
                break;
            if ( ( ! shortened ) || ( b == 0 ) )
                g_string_append_c(addresses, ':');
        }

        if (g_slist_next(address_) != NULL )
            g_string_append(addresses, ", ");
    }
    g_string_append(addresses, " ");
}

static void
_j4status_nm_device_get_addresses(J4statusPluginContext *context, NMDevice *device, GString *addresses)
{
    _j4status_nm_device_get_addresses_ipv4(context, device, addresses);
    _j4status_nm_device_get_addresses_ipv6(context, device, addresses);
}

static void
_j4status_nm_device_update(J4statusPluginContext *context, J4statusNmSection *section, NMDevice *device)
{

    J4statusState state = J4STATUS_STATE_NO_STATE;
    gchar *value = NULL;
    switch ( nm_device_get_state(device) )
    {
    case NM_DEVICE_STATE_UNKNOWN:
        state = J4STATUS_STATE_NO_STATE;
        value = context->show_unknown ? g_strdup("Unknown") : NULL;
    break;
    case NM_DEVICE_STATE_UNMANAGED:
        state = J4STATUS_STATE_NO_STATE;
        value = context->show_unmanaged ? g_strdup("Unmanaged") : NULL;
    break;
    case NM_DEVICE_STATE_UNAVAILABLE:
        state = J4STATUS_STATE_UNAVAILABLE;
        value = context->hide_unavailable ? NULL : g_strdup("Unavailable");
    break;
    case NM_DEVICE_STATE_DISCONNECTED:
        state = J4STATUS_STATE_BAD;
        switch ( nm_device_get_device_type(device) )
        {
        case NM_DEVICE_TYPE_WIFI:
        {
            gchar *aps_number = NULL;
            if ( context->wifi.show_available_aps_number )
            {
                const GPtrArray *aps;
                aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));
                if ( aps != NULL )
                    aps_number = g_strdup_printf(" (%u APs)", aps->len);
            }
            value = g_strdup_printf("Down%s", ( aps_number != NULL ) ? aps_number : "");
            g_free(aps_number);
        }
        break;
        default:
            value = g_strdup("Down");
        break;
        }
    break;
    case NM_DEVICE_STATE_PREPARE:
        state = J4STATUS_STATE_AVERAGE;
        value = g_strdup("Prepare");
    break;
    case NM_DEVICE_STATE_CONFIG:
        state = J4STATUS_STATE_AVERAGE;
        value = g_strdup("Config");
    break;
    case NM_DEVICE_STATE_NEED_AUTH:
        state = J4STATUS_STATE_AVERAGE;
        value = g_strdup("Need auth");
    break;
    case NM_DEVICE_STATE_IP_CONFIG:
        state = J4STATUS_STATE_GOOD;
        value = g_strdup("IP configuration");
    break;
    case NM_DEVICE_STATE_IP_CHECK:
        state = J4STATUS_STATE_GOOD;
        value = g_strdup("IP check");
    break;
    case NM_DEVICE_STATE_SECONDARIES:
        state = J4STATUS_STATE_AVERAGE;
        value = g_strdup("Secondaries");
    break;
    case NM_DEVICE_STATE_ACTIVATED:
    {
        GString *addresses;

        addresses = g_string_new("");
        _j4status_nm_device_get_addresses(context, device, addresses);

        switch ( nm_device_get_device_type(device) )
        {
        case NM_DEVICE_TYPE_WIFI:
        {
            state = J4STATUS_STATE_AVERAGE;

            gchar *ap_text = NULL;
            if ( section->ap != NULL )
            {
                guint8 strength;
                strength =  nm_access_point_get_strength(section->ap);
                if ( strength > 75 )
                    state = J4STATUS_STATE_GOOD;
                else if ( strength < 25 )
                    state = J4STATUS_STATE_BAD;

                const GByteArray *raw_ssid;
                raw_ssid = nm_access_point_get_ssid(section->ap);
                ap_text = g_strdup_printf("%03u%% at %.*s, ", strength, raw_ssid->len, (const gchar *)raw_ssid->data);
            }

            guint32 bitrate;
            bitrate = nm_device_wifi_get_bitrate(NM_DEVICE_WIFI(device));

            value = g_strdup_printf("%s(%s%uMb/s)", addresses->str, ( ap_text != NULL ) ? ap_text : "", bitrate/1000);

            g_free(ap_text);
            g_string_free(addresses, TRUE);
        }
        break;
        case NM_DEVICE_TYPE_ETHERNET:
        {
            state = J4STATUS_STATE_GOOD;

            guint32 speed;
            speed = nm_device_ethernet_get_speed(NM_DEVICE_ETHERNET(device));

            if ( speed == 0 )
            {
                g_string_truncate(addresses, addresses->len - 1);
                value = g_string_free(addresses, FALSE);
            }
            else
            {
                    if ( ( speed % 1000 ) == 0 )
                    value = g_strdup_printf("%s(%uGb/s)", addresses->str, speed/1000);
                else
                    value = g_strdup_printf("%s(%uMb/s)", addresses->str, speed);

                g_string_free(addresses, TRUE);
            }
        }
        break;
        default:
            state = J4STATUS_STATE_GOOD;
            value = g_string_free(addresses, FALSE);
        break;
        }
    }
    break;
    case NM_DEVICE_STATE_DEACTIVATING:
        state = J4STATUS_STATE_BAD;
        value = g_strdup("Disconnecting");
    break;
    case NM_DEVICE_STATE_FAILED:
        state = J4STATUS_STATE_BAD;
        value = g_strdup("Failed");
    break;
    }

    j4status_section_set_state(section->section, state);
    j4status_section_set_value(section->section, value);

    libj4status_core_trigger_display(context->core);
}

static void
_j4status_nm_access_point_property_changed(NMAccessPoint *device, GParamSpec *pspec, gpointer user_data)
{
    J4statusNmSection *section = user_data;
    _j4status_nm_device_update(section->context, section, section->device);
}

static void
_j4status_nm_device_property_changed(NMDevice *device, GParamSpec *pspec, gpointer user_data)
{
    J4statusNmSection *section = user_data;
    if ( g_strcmp0("active-access-point", pspec->name) == 0 )
    {
        if ( section->ap != NULL )
        {
            g_signal_handler_disconnect(section->ap, section->strength_handler);
            g_object_unref(section->ap);
        }
        section->ap = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
        if ( section->ap != NULL )
            section->strength_handler = g_signal_connect(g_object_ref(section->ap), "notify::strength", G_CALLBACK(_j4status_nm_access_point_property_changed), section);
    }
    _j4status_nm_device_update(section->context, section, device);
}

static void
_j4status_nm_device_state_changed(NMDevice *device, guint state, guint arg2, guint arg3, gpointer user_data)
{
    J4statusNmSection *section = user_data;
    _j4status_nm_device_update(section->context, section, device);
}

static void
_j4status_nm_device_monitor(gpointer key, gpointer data, gpointer user_data)
{
    J4statusNmSection *section = data;

    switch ( nm_device_get_device_type(section->device) )
    {
    case NM_DEVICE_TYPE_WIFI:
        section->bitrate_handler = g_signal_connect(section->device, "notify::bitrate", G_CALLBACK(_j4status_nm_device_property_changed), section);
        section->active_access_point_handler = g_signal_connect(section->device, "notify::active-access-point", G_CALLBACK(_j4status_nm_device_property_changed), section);
        if ( section->ap != NULL )
            section->strength_handler = g_signal_connect(section->ap, "notify::strength", G_CALLBACK(_j4status_nm_access_point_property_changed), section);
    break;
    default:
    break;
    }
    section->state_changed_handler = g_signal_connect(section->device, "state-changed", G_CALLBACK(_j4status_nm_device_state_changed), section);
    _j4status_nm_device_update(section->context, section, section->device);
}

static void
_j4status_nm_device_unmonitor(gpointer key, gpointer data, gpointer user_data)
{
    J4statusNmSection *section = data;

    switch ( nm_device_get_device_type(section->device) )
    {
    case NM_DEVICE_TYPE_WIFI:
        g_signal_handler_disconnect(section->device, section->bitrate_handler);
        g_signal_handler_disconnect(section->device, section->active_access_point_handler);
        if ( section->ap != NULL )
            g_signal_handler_disconnect(section->ap, section->strength_handler);
    break;
    default:
    break;
    }
    g_signal_handler_disconnect(section->device, section->state_changed_handler);
}

static void
_j4status_nm_section_attach_device(J4statusPluginContext *context, J4statusNmSection *section, NMDevice *device)
{
    section->device = g_object_ref(device);

    const gchar *name;
    const gchar *label;
    switch ( nm_device_get_device_type(device) )
    {
    case NM_DEVICE_TYPE_UNKNOWN:
    case NM_DEVICE_TYPE_UNUSED1:
    case NM_DEVICE_TYPE_UNUSED2:
        name = "nm-unknown";
        label = "Unknownw";
    break;
    case NM_DEVICE_TYPE_ETHERNET:
        name = "nm-ethernet";
        label = "E";
    break;
    case NM_DEVICE_TYPE_WIFI:
        name = "nm-wifi";
        label = "W";
        section->ap = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
        if ( section->ap != NULL )
            g_object_ref(section->ap);
    break;
    case NM_DEVICE_TYPE_BT:
        name = "nm-bluetooth";
        label = "B";
    break;
    case NM_DEVICE_TYPE_OLPC_MESH:
        name = "nm-olpc-mesh";
        label = "OM";
    break;
    case NM_DEVICE_TYPE_WIMAX:
        name = "nm-wimax";
        label = "Wx";
    break;
    case NM_DEVICE_TYPE_MODEM:
        name = "nm-modem";
        label = "M";
    break;
    case NM_DEVICE_TYPE_INFINIBAND:
        name = "nm-infiniband";
        label = "I";
    break;
    case NM_DEVICE_TYPE_BOND:
        name = "nm-bond";
        label = "Bo";
    break;
    case NM_DEVICE_TYPE_VLAN:
        name = "nm-vlan";
        label = "V";
    break;
#if NM_CHECK_VERSION(0,9,5)
    case NM_DEVICE_TYPE_ADSL:
        name = "nm-adsl";
        label = "A";
    break;
#endif /* NM_CHECK_VERSION(0.9.5) */
    default:
        g_warning("Unsupported device type for interface '%s'", section->interface);
        return;
    }
    section->section = j4status_section_new(context->core, name, section->interface);
    j4status_section_set_label(section->section, label);

    if ( context->started )
        _j4status_nm_device_monitor(NULL, section, NULL);
}

static void
_j4status_nm_section_detach_device(J4statusNmSection *section)
{
    g_object_unref(section->device);
    section->device = NULL;
    j4status_section_free(section->section);
    section->section = NULL;
}

static void
_j4status_nm_client_device_added(NMClient *client, NMDevice *device, gpointer user_data)
{
    J4statusPluginContext *context = user_data;
    J4statusNmSection *section;
    section = g_hash_table_lookup(context->sections, nm_device_get_iface(device));
    if ( section != NULL )
        _j4status_nm_section_attach_device(context, section, device);
}

static void
_j4status_nm_client_device_removed(NMClient *client, NMDevice *device, gpointer user_data)
{
    J4statusPluginContext *context = user_data;
    J4statusNmSection *section;
    section = g_hash_table_lookup(context->sections, nm_device_get_iface(device));
    if ( section != NULL )
        _j4status_nm_section_detach_device(section);
}

static void
_j4status_nm_section_new(J4statusPluginContext *context, gchar *interface)
{
    J4statusNmSection *section;
    section = g_new0(J4statusNmSection, 1);
    section->context = context;
    section->interface = interface;

    g_hash_table_insert(context->sections, interface, section);

    NMDevice *device;
    device = nm_client_get_device_by_iface(context->nm_client, interface);
    if ( device != NULL )
        _j4status_nm_section_attach_device(context, section, device);
}

static void
_j4status_nm_section_free(gpointer data)
{
    J4statusNmSection *section = data;

    if ( section->device != NULL )
        g_object_unref(section->device);

    j4status_section_free(section->section);

    g_free(section);
}

static J4statusPluginContext *
_j4status_nm_init(J4statusCoreInterface *core)
{
    GKeyFile *key_file;
    key_file = libj4status_config_get_key_file("NetworkManager");
    if ( key_file == NULL )
        return NULL;

    gchar **interfaces;
    interfaces = g_key_file_get_string_list(key_file, "NetworkManager", "Interfaces", NULL, NULL);
    if ( interfaces == NULL )
    {
        g_key_file_free(key_file);
        return NULL;
    }

    J4statusPluginContext *context;
    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->sections = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _j4status_nm_section_free);

    context->show_unknown = g_key_file_get_boolean(key_file, "NetworkManager", "ShowUnknown", NULL);
    context->show_unmanaged = g_key_file_get_boolean(key_file, "NetworkManager", "ShowUnmanaged", NULL);
    context->hide_unavailable = g_key_file_get_boolean(key_file, "NetworkManager", "HideUnavailable", NULL);

    context->wifi.show_available_aps_number = g_key_file_get_boolean(key_file, "NetworkManager", "WiFi-ShowAvailableAPsNumber", NULL);

    g_key_file_free(key_file);

    context->nm_client = nm_client_new();

    gchar **interface;
    for ( interface = interfaces ; *interface != NULL ; ++interface )
        _j4status_nm_section_new(context, *interface);
    g_free(interfaces);

    g_signal_connect(context->nm_client, "device-added", G_CALLBACK(_j4status_nm_client_device_added), context);
    g_signal_connect(context->nm_client, "device-removed", G_CALLBACK(_j4status_nm_client_device_removed), context);

    return context;
}

static void
_j4status_nm_uninit(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    g_hash_table_unref(context->sections);

    g_object_unref(context->nm_client);

    g_free(context);
}

static void
_j4status_nm_start(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    g_hash_table_foreach(context->sections, _j4status_nm_device_monitor, NULL);
    context->started = TRUE;
}

static void
_j4status_nm_stop(J4statusPluginContext *context)
{
    if ( context == NULL )
        return;

    context->started = FALSE;
    g_hash_table_foreach(context->sections, _j4status_nm_device_unmonitor, NULL);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_nm_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_nm_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_nm_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_nm_stop);
}
