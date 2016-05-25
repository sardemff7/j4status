/*
 * j4status - Status line generator
 *
 * Copyright © 2012-201È Quentin "Sardem FF7" Glidic
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

#include <sys/socket.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/socket.h>
#include <netlink/msg.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <libgwater-nl.h>

#include <j4status-plugin-input.h>

#define BUFFER_MAX 500

typedef enum {
    ADDRESSES_IPV4,
    ADDRESSES_IPV6,
    ADDRESSES_ALL,
} J4statusNlAddresses;

static const gchar * const _j4status_nl_addresses[] = {
    [ADDRESSES_IPV4] = "ipv4",
    [ADDRESSES_IPV6] = "ipv6",
    [ADDRESSES_ALL]  = "all",
};

typedef enum {
    TOKEN_UP_ADDRESSES,
} J4statusNlFormatUpToken;

static const gchar * const _j4status_nl_format_up_tokens[] = {
    [TOKEN_UP_ADDRESSES] = "addresses",
};

#define J4STATUS_NL_DEFAULT_UP_FORMAT "${addresses}"
#define J4STATUS_NL_DEFAULT_DOWN_FORMAT "Down"

struct _J4statusPluginContext {
    GHashTable *sections;
    GWaterNlSource *source;
    struct nl_sock *sock;
    struct nl_cache_mngr *cache_mngr;
    struct nl_cache *link_cache;
    struct nl_cache *addr_cache;
    gboolean started;

    J4statusNlAddresses addresses;
    struct {
        J4statusFormatString *up;
        J4statusFormatString *down;
    } formats;
};

typedef struct {
    J4statusPluginContext *context;
    J4statusSection *section;
    gint ifindex;
    struct rtnl_link *link;
    GList *ipv4_addresses;
    GList *ipv6_addresses;
} J4statusNlSection;

static const gchar *
_j4status_nl_format_up_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    switch ( value )
    {
    case TOKEN_UP_ADDRESSES:
        return user_data;
    break;
    default:
        g_assert_not_reached();
    }
    return NULL;
}

static gsize
_j4status_nl_section_append_addresses(gchar *str, gsize size, GList *list)
{
    gsize o = 0;
    GList *addr;
    for ( addr = list ; addr != NULL ; addr = g_list_next(addr) )
    {
        gchar *p;
        nl_addr2str(rtnl_addr_get_local(addr->data), str + o, size - o);
        p = g_utf8_strchr(str + o, -1, '/');
        g_assert_nonnull(p); /* We know libnl wrote the prefix length */
        o = p - str;
        o += g_snprintf(str + o, size - o, ", ");
    }
    return o;
}

static gchar *
_j4status_nl_section_get_addresses(J4statusNlSection *self)
{
    gsize size = 0, o = 0;
    gchar *addresses;

    if ( self->context->addresses != ADDRESSES_IPV6 )
        size += g_list_length(self->ipv4_addresses) * strlen("255.255.255.255, ");
    if ( self->context->addresses != ADDRESSES_IPV4 )
        size += g_list_length(self->ipv6_addresses) * strlen("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff, ");
    size += strlen("/128"); /* We will truncate prefix length, but we need room for libnl to write it first */

    addresses = g_new(char, size);

    if ( self->context->addresses != ADDRESSES_IPV6 )
        o += _j4status_nl_section_append_addresses(addresses + o, size - o, self->ipv4_addresses);

    if ( self->context->addresses != ADDRESSES_IPV4 )
        o += _j4status_nl_section_append_addresses(addresses + o, size - o, self->ipv6_addresses);

    /* Strip the last separator */
    addresses[o-2] = '\0';

    return addresses;
}

static void
_j4status_nl_section_free_addresses(J4statusNlSection *self)
{
    g_list_free_full(self->ipv4_addresses, (GDestroyNotify) nl_addr_put);
    g_list_free_full(self->ipv6_addresses, (GDestroyNotify) nl_addr_put);
    self->ipv4_addresses = NULL;
    self->ipv6_addresses = NULL;
}

static void
_j4status_nl_section_update(J4statusNlSection *self)
{
    if ( self->link == NULL )
        return;

    guint flags;
    flags = rtnl_link_get_flags(self->link);

    gchar *value = NULL;
    J4statusState state = J4STATUS_STATE_NO_STATE;
    if ( flags & IFF_LOWER_UP )
    {
        if ( ( self->ipv4_addresses == NULL ) && ( self->ipv6_addresses == NULL ) )
        {
            state = J4STATUS_STATE_AVERAGE;
            value = g_strdup("Connecting");
        }
        else
        {
            gchar *addresses;
            addresses = _j4status_nl_section_get_addresses(self);

            state = J4STATUS_STATE_GOOD;
            value = j4status_format_string_replace(self->context->formats.up, _j4status_nl_format_up_callback, addresses);

            g_free(addresses);
        }
    }
    else if ( flags & (IFF_UP | IFF_RUNNING) )
    {
        state = J4STATUS_STATE_BAD;
        value = g_strdup("Not connected");
        _j4status_nl_section_free_addresses(self);
    }

    j4status_section_set_state(self->section, state);
    j4status_section_set_value(self->section, value);
}

static void
_j4status_nl_section_free(gpointer data)
{
    J4statusNlSection *self = data;

    j4status_section_free(self->section);

    _j4status_nl_section_free_addresses(self);

    rtnl_link_put(self->link);

    g_free(self);
}

static J4statusNlSection *
_j4status_nl_section_new(J4statusPluginContext *context, J4statusCoreInterface *core, const gchar *interface)
{
    struct rtnl_link *link;
    link = rtnl_link_get_by_name(context->link_cache, interface);
    if ( link == NULL )
    {
        g_warning("Couldn't get interface %s", interface);
        return NULL;
    }
    const gchar *name = NULL;
    switch ( rtnl_link_get_arptype(link) )
    {
    case ARPHRD_ETHER:
        name = "nl-ether";
    break;
    case ARPHRD_IEEE80211:
        name = "nl-802.11";
    break;
    default:
        g_warning("Interface %s has an unsupported type", interface);
        rtnl_link_put(link);
        return NULL;
    }

    J4statusNlSection *self;

    self = g_new0(J4statusNlSection, 1);
    self->context = context;
    self->ifindex = rtnl_link_get_ifindex(link);
    self->link = link;

    self->section = j4status_section_new(core);

    j4status_section_set_name(self->section, name);
    j4status_section_set_instance(self->section, interface);
    j4status_section_set_label(self->section, interface);

    if ( ! j4status_section_insert(self->section) )
    {
        _j4status_nl_section_free(self);
        return NULL;
    }

    _j4status_nl_section_update(self);

    return self;
}

static gint
_j4status_nl_address_compare(gconstpointer a, gconstpointer b)
{
    if ( a == b )
        return 0;
    return nl_addr_cmp(a, b);
}

static void
_j4status_nl_cache_change(struct nl_cache *cache, struct nl_object *object, int something, void *user_data)
{
    J4statusPluginContext *self = user_data;
        J4statusNlSection *section;

    if ( cache == self->link_cache )
    {
        struct rtnl_link *link = nl_object_priv(object);

        section = g_hash_table_lookup(self->sections, GINT_TO_POINTER(rtnl_link_get_ifindex(link)));
        if ( section == NULL )
            return;
        g_debug("Link cache update: %p = %s", object, rtnl_link_get_ifalias(link));

        if ( section->link != link )
        {
            nl_object_get(object);
            rtnl_link_put(section->link);
            section->link = link;
        }
    }
    else if ( cache == self->addr_cache )
    {
        struct rtnl_addr *addr = nl_object_priv(object);
        section = g_hash_table_lookup(self->sections, GINT_TO_POINTER(rtnl_addr_get_ifindex(addr)));
        if ( section == NULL )
            return;
        g_debug("Addr cache update: %p = %d", object, rtnl_addr_get_ifindex(addr));

        GList **list;
        switch ( rtnl_addr_get_family(addr) )
        {
        case AF_INET:
            list = &section->ipv4_addresses;
        break;
        case AF_INET6:
            list = &section->ipv6_addresses;
        break;
        default:
            /* Not supported */
            return;
        }

        if ( g_list_find_custom(*list, addr,_j4status_nl_address_compare)  != NULL )
            /* Already got it */
            return;

        nl_object_get(object);
        *list = g_list_prepend(*list, addr);
    }
    else
        g_assert_not_reached();
    _j4status_nl_section_update(section);
}


static void _j4status_nl_uninit(J4statusPluginContext *self);

static J4statusPluginContext *
_j4status_nl_init(J4statusCoreInterface *core)
{
    gchar **interfaces = NULL;
    guint64 addresses = ADDRESSES_ALL;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("Netlink");
    if ( key_file != NULL )
    {
        interfaces = g_key_file_get_string_list(key_file, "Netlink", "Interfaces", NULL, NULL);
        j4status_config_key_file_get_enum(key_file, "Netlink", "Addresses", _j4status_nl_addresses, G_N_ELEMENTS(_j4status_nl_addresses), &addresses);

        g_key_file_free(key_file);
    }

    if ( interfaces == NULL )
    {
        g_message("No interface to monitor, aborting");
        return NULL;
    }

    gint err = 0;
    J4statusPluginContext *self;

    self = g_new0(J4statusPluginContext, 1);
    self->addresses = addresses;

    self->sections = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, _j4status_nl_section_free);

    self->source = g_water_nl_source_new_cache_mngr(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &err);
    if ( self->source == NULL )
    {
        g_warning("Couldn't subscribe to events: %s", nl_geterror(err));
        goto error;
    }
    self->sock = g_water_nl_source_get_sock(self->source);
    self->cache_mngr = g_water_nl_source_get_cache_mngr(self->source);

    err = rtnl_link_alloc_cache(self->sock, AF_UNSPEC, &self->link_cache);
    if ( err < 0 )
    {
        g_warning("Couldn't allocate links cache: %s", nl_geterror(err));
        goto error;
    }

    err = nl_cache_mngr_add_cache(self->cache_mngr, self->link_cache, _j4status_nl_cache_change, self);
    if ( err < 0 )
    {
        g_warning("Couldn't manage links cache: %s", nl_geterror(err));
        goto error;
    }

    err = rtnl_addr_alloc_cache(self->sock, &self->addr_cache);
    if ( err < 0 )
    {
        g_warning("Couldn't allocate addresses cache: %s", nl_geterror(err));
        goto error;
    }

    err = nl_cache_mngr_add_cache(self->cache_mngr, self->addr_cache, _j4status_nl_cache_change, self);
    if ( err < 0 )
    {
        g_warning("Couldn't manage addresses cache: %s", nl_geterror(err));
        goto error;
    }

    gchar **interface;
    for ( interface = interfaces ; *interface != NULL ; ++interface )
    {
        J4statusNlSection *section;
        section = _j4status_nl_section_new(self, core, *interface);
        if ( section != NULL )
            g_hash_table_insert(self->sections, GINT_TO_POINTER(section->ifindex), section);
        else
            g_free(*interface);
    }

    g_free(interfaces);


    if ( g_hash_table_size(self->sections) < 1 )
        goto error;

    self->formats.up   = j4status_format_string_parse(NULL, _j4status_nl_format_up_tokens, G_N_ELEMENTS(_j4status_nl_format_up_tokens), J4STATUS_NL_DEFAULT_UP_FORMAT,   NULL);
    self->formats.down = j4status_format_string_parse(NULL, NULL,                          0,                                           J4STATUS_NL_DEFAULT_DOWN_FORMAT, NULL);


    return self;

error:
    _j4status_nl_uninit(self);
    return NULL;
}

static void
_j4status_nl_uninit(J4statusPluginContext *self)
{
    nl_cache_put(self->addr_cache);
    nl_cache_put(self->link_cache);

    if ( self->source != NULL )
        g_water_nl_source_unref(self->source);

    g_hash_table_unref(self->sections);

    g_free(self);
}

static void
_j4status_nl_start(J4statusPluginContext *self)
{
    self->started = TRUE;
}

static void
_j4status_nl_stop(J4statusPluginContext *self)
{
    self->started = FALSE;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_nl_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_nl_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_nl_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_nl_stop);
}
