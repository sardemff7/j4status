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
#include <linux/nl80211.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/socket.h>
#include <netlink/msg.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <libgwater-nl.h>

#include <j4status-plugin-input.h>

#ifndef NUM_NL80211_ATTR
#define NUM_NL80211_ATTR (NL80211_ATTR_MAX+1)
#endif /* ! NUM_NL80211_ATTR */

#ifndef NL80211_MULTICAST_GROUP_CONFIG
#define NL80211_MULTICAST_GROUP_CONFIG "config"
#endif /* ! NL80211_MULTICAST_GROUP_CONFIG */

#ifndef NL80211_MULTICAST_GROUP_SCAN
#define NL80211_MULTICAST_GROUP_SCAN "scan"
#endif /* ! NL80211_MULTICAST_GROUP_SCAN */

#ifndef NL80211_MULTICAST_GROUP_MLME
#define NL80211_MULTICAST_GROUP_MLME "mlme"
#endif /* ! NL80211_MULTICAST_GROUP_MLME */

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

typedef enum {
    TOKEN_UP_WIFI_ADDRESSES,
    TOKEN_UP_WIFI_STRENGTH,
    TOKEN_UP_WIFI_SSID,
    TOKEN_UP_WIFI_BITRATE,
} J4statusNlFormatUpWiFiToken;

typedef enum {
    TOKEN_DOWN_WIFI_APS,
} J4statusNlFormatDownWiFiToken;

static const gchar * const _j4status_nl_format_up_tokens[] = {
    [TOKEN_UP_ADDRESSES] = "addresses",
};

static const gchar * const _j4status_nl_format_up_wifi_tokens[] = {
    [TOKEN_UP_WIFI_ADDRESSES] = "addresses",
    [TOKEN_UP_WIFI_STRENGTH]  = "strength",
    [TOKEN_UP_WIFI_SSID]      = "ssid",
    [TOKEN_UP_WIFI_BITRATE]   = "bitrate",
};

static const gchar * const _j4status_nl_format_down_wifi_tokens[] = {
    [TOKEN_DOWN_WIFI_APS] = "aps",
};

typedef enum {
    TOKEN_FLAG_UP_ADDRESSES = (1 << TOKEN_UP_ADDRESSES),
} J4statusNlFormatUpEthTokenFlag;

typedef enum {
    TOKEN_FLAG_UP_WIFI_ADDRESSES = (1 << TOKEN_UP_WIFI_ADDRESSES),
    TOKEN_FLAG_UP_WIFI_STRENGTH  = (1 << TOKEN_UP_WIFI_STRENGTH),
    TOKEN_FLAG_UP_WIFI_SSID      = (1 << TOKEN_UP_WIFI_SSID),
    TOKEN_FLAG_UP_WIFI_BITRATE   = (1 << TOKEN_UP_WIFI_BITRATE),
} J4statusNlFormatUpWiFiTokenFlag;

typedef enum {
    TOKEN_FLAG_DOWN_WIFI_APS = (1 << TOKEN_DOWN_WIFI_APS),
} J4statusNlFormatDownWiFiTokenFlag;

#define J4STATUS_NL_DEFAULT_FORMAT_UP "${addresses}"
#define J4STATUS_NL_DEFAULT_FORMAT_DOWN "Down"
#define J4STATUS_NL_DEFAULT_FORMAT_UP_WIFI "${addresses} (${strength}${strength:+% }${ssid/^.+$/at \\0, }${bitrate:+${bitrate(p)}b/s})"
#define J4STATUS_NL_DEFAULT_FORMAT_DOWN_WIFI "Down${aps/^.+$/(\\0 APs)}"

typedef struct {
    int error;
    struct nlattr **answer;
    int size;
    gsize number;
} J4statusNlMessageAnswer;

struct _J4statusPluginContext {
    GHashTable *sections;
    GWaterNlSource *source;
    struct nl_sock *sock;
    struct nl_cache_mngr *cache_mngr;
    struct nl_cache *link_cache;
    struct nl_cache *addr_cache;
    struct {
        J4statusNlMessageAnswer data;
        GWaterNlSource *source;
        struct nl_sock *sock;
        int id;
        GWaterNlSource *esource;
        struct nl_sock *esock;
    } nl80211;
    gboolean started;

    J4statusNlAddresses addresses;
    struct {
        J4statusFormatString *up;
        J4statusFormatString *down;
        J4statusFormatString *up_wifi;
        J4statusFormatString *down_wifi;
        guint64 up_tokens;
        guint64 up_wifi_tokens;
        guint64 down_wifi_tokens;
    } formats;
};

typedef struct {
    J4statusPluginContext *context;
    J4statusSection *section;
    gint ifindex;
    struct rtnl_link *link;
    struct {
        gboolean is;
        gboolean has_ap;
        gchar *ssid;
        gint8 strength;
        guint64 bitrate;
        gint64 aps;
    } wifi;
    struct {
        gboolean has;
        GList *ipv4;
        GList *ipv6;
    } addresses;
} J4statusNlSection;

static int
_j4status_nl_message_error_callback(struct sockaddr_nl *nla, struct nlmsgerr *error, void *user_data)
{
    J4statusNlMessageAnswer *data = user_data;
    data->error = error->error;
    return NL_STOP;
}

static int
_j4status_nl_message_ack_callback(struct nl_msg *msg, void *user_data)
{
    J4statusNlMessageAnswer *data = user_data;
    data->error = 0;
    return NL_STOP;
}

static int
_j4status_nl_message_valid_callback(struct nl_msg *msg, void *user_data)
{
    J4statusNlMessageAnswer *data = user_data;
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

    ++data->number;
    nla_parse(data->answer, data->size, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    return NL_SKIP;
}

static int
_j4status_nl_send_message(J4statusPluginContext *self, struct nl_msg *message, struct nlattr **answer, int size, gsize *number)
{
    self->nl80211.data.error = 1;
    self->nl80211.data.answer = answer;
    self->nl80211.data.size = size;
    self->nl80211.data.number = 0;

    int err;
    err = nl_send_auto_complete(self->nl80211.sock, message);
    if ( err < 0 )
    {
        g_warning("Couldn’t send message: %s", nl_geterror(err));
        return 1;
    }

    while ( self->nl80211.data.error > 0 )
        nl_recvmsgs_default(self->nl80211.sock);

    if ( number != NULL )
        *number = self->nl80211.data.number;

    return self->nl80211.data.error;
}

static gboolean
_j4status_nl_register_events(J4statusPluginContext *self)
{
    struct nl_msg *message;

    message = nlmsg_alloc();
    if ( message == NULL )
        return FALSE;

    gboolean ret = FALSE;

    int ctrlid;
    ctrlid = genl_ctrl_resolve(self->nl80211.esock, "nlctrl");

    genlmsg_put(message, NL_AUTO_PORT, NL_AUTO_SEQ, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);
    NLA_PUT_STRING(message, CTRL_ATTR_FAMILY_NAME, NL80211_GENL_NAME);

    struct nlattr *answer[CTRL_ATTR_MAX + 1];
    int err;
    if ( ( err = _j4status_nl_send_message(self, message, answer, CTRL_ATTR_MAX, NULL) ) != 0 )
    {
        if ( err < 0 )
            g_warning("Couldn’t get multicast groups ids: %s", nl_geterror(err));
        goto fail;
    }

    if ( answer[CTRL_ATTR_MCAST_GROUPS] == NULL )
        goto fail;

    struct nlattr *group;
    int rem;
    nla_for_each_nested(group, answer[CTRL_ATTR_MCAST_GROUPS], rem)
    {
        struct nlattr *group_attr[CTRL_ATTR_MCAST_GRP_MAX + 1];

        nla_parse(group_attr, CTRL_ATTR_MCAST_GRP_MAX, nla_data(group), nla_len(group), NULL);

        if ( ( group_attr[CTRL_ATTR_MCAST_GRP_NAME] == NULL ) || ( group_attr[CTRL_ATTR_MCAST_GRP_ID] == NULL ) )
            continue;

        const char *name = nla_data(group_attr[CTRL_ATTR_MCAST_GRP_NAME]);
        size_t n = nla_len(group_attr[CTRL_ATTR_MCAST_GRP_NAME]);
        if ( ( strncmp(name, NL80211_MULTICAST_GROUP_CONFIG, n) != 0 )
             && ( strncmp(name, NL80211_MULTICAST_GROUP_MLME, n) != 0 )
             && ( strncmp(name, NL80211_MULTICAST_GROUP_SCAN, n) != 0 ) )
            continue;

        int id = nla_get_u32(group_attr[CTRL_ATTR_MCAST_GRP_ID]);
        err = nl_socket_add_membership(self->nl80211.esock, id);
        if ( err < 0 )
        {
            ret = FALSE;
            g_warning("Couldn’t register to %.*s events: %s", (int) n, name, nl_geterror(err));
            goto fail;
        }
        ret = TRUE;
    }

nla_put_failure:
fail:
    nlmsg_free(message);
    return ret;
}

static void
_j4status_nl_section_update_nl80211(J4statusNlSection *self)
{
    g_return_if_fail(self->wifi.is);

    self->wifi.has_ap = FALSE;
    g_free(self->wifi.ssid);
    self->wifi.ssid = NULL;
    self->wifi.strength = -1;
    self->wifi.bitrate = 0;
    self->wifi.aps = -1;

    struct nl_msg *message;

    message = nlmsg_alloc();
    genlmsg_put(message, NL_AUTO_PORT, NL_AUTO_SEQ, self->context->nl80211.id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
    NLA_PUT_U32(message, NL80211_ATTR_IFINDEX, self->ifindex);

    int err;
    struct nlattr *answer[NUM_NL80211_ATTR] = { NULL };
    gsize aps;
    if ( ( err = _j4status_nl_send_message(self->context, message, answer, NL80211_ATTR_MAX, &aps) ) != 0 )
    {
        if ( err < 0 )
            g_warning("Couldn’t query nl80211 scan information: %s", nl_geterror(err));
        goto end;
    }

    self->wifi.aps = aps;

    if ( answer[NL80211_ATTR_BSS] == NULL )
        goto end;
    struct nlattr *bss[NL80211_BSS_MAX + 1] = { NULL };
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
        [NL80211_BSS_BSSID] = { },
        [NL80211_BSS_INFORMATION_ELEMENTS] = { },
        [NL80211_BSS_STATUS] = { .type = NLA_U32 },
    };
    if ( nla_parse_nested(bss, NL80211_BSS_MAX, answer[NL80211_ATTR_BSS], bss_policy) < 0 )
        goto end;

    if ( bss[NL80211_BSS_STATUS] == NULL )
        goto end;

    if ( bss[NL80211_BSS_INFORMATION_ELEMENTS] != NULL )
    {
        guchar *data = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
        gint len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
        guchar *c;
        guchar size;
        for ( c = data ; c < data + len ; c += size )
        {
            gint type = c[0];
            size = c[1];
            c += 2;

            switch ( type )
            {
            case 0: /* SSID */
                self->wifi.ssid = g_strndup((gchar *) c, size);
            break;
            }
        }
    }

    switch ( nla_get_u32(bss[NL80211_BSS_STATUS]) )
    {
    case NL80211_BSS_STATUS_ASSOCIATED:
    break;
    case NL80211_BSS_STATUS_AUTHENTICATED:
        goto has_ap;
    case NL80211_BSS_STATUS_IBSS_JOINED:
        goto has_ap;
    default:
        goto end;
    }

    nlmsg_free(message);
    message = nlmsg_alloc();
    genlmsg_put(message, NL_AUTO_PORT, NL_AUTO_SEQ, self->context->nl80211.id, 0, 0, NL80211_CMD_GET_STATION, 0);
    NLA_PUT_U32(message, NL80211_ATTR_IFINDEX, self->ifindex);
    NLA_PUT(message, NL80211_ATTR_MAC, nla_len(bss[NL80211_BSS_BSSID]), nla_data(bss[NL80211_BSS_BSSID]));
    if ( ( err = _j4status_nl_send_message(self->context, message, answer, NL80211_ATTR_MAX, NULL) ) != 0 )
    {
        if ( err < 0 )
            g_warning("Couldn’t query nl80211 station: %s", nl_geterror(err));
        goto end;
    }

    if ( answer[NL80211_ATTR_STA_INFO] == NULL )
        goto end;

    struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
    static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
        [NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
        [NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
    };
    if ( nla_parse_nested(sinfo, NL80211_STA_INFO_MAX, answer[NL80211_ATTR_STA_INFO], stats_policy) < 0 )
        goto end;

    if ( sinfo[NL80211_STA_INFO_TX_BITRATE] != NULL )
    {
        struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
        static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
                [NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
                [NL80211_RATE_INFO_BITRATE32] = { .type = NLA_U32 },
        };

        if ( nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX, sinfo[NL80211_STA_INFO_TX_BITRATE], rate_policy) < 0 )
            goto end;

        gint64 rate = 0;
        if ( rinfo[NL80211_RATE_INFO_BITRATE32] != NULL )
            rate = nla_get_u32(rinfo[NL80211_RATE_INFO_BITRATE32]);
        else if ( rinfo[NL80211_RATE_INFO_BITRATE] != NULL )
            rate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);
        if ( rate > 0 )
            self->wifi.bitrate = rate * 100 * 1000;
    }

    if ( sinfo[NL80211_STA_INFO_SIGNAL] != NULL )
    {
        gint8 dbm = (gint8) nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);
        self->wifi.strength = ( 100 + CLAMP(dbm, -100, -50) ) * 2;
    }

has_ap:
    self->wifi.has_ap = TRUE;

nla_put_failure:
end:
    nlmsg_free(message);
}

static gboolean
_j4status_nl_section_check_nl80211(J4statusNlSection *self, const gchar *interface)
{
    gboolean ret = FALSE;
    struct nl_msg *message;

    message = nlmsg_alloc();
    genlmsg_put(message, NL_AUTO_PORT, NL_AUTO_SEQ, self->context->nl80211.id, 0, 0, NL80211_CMD_GET_INTERFACE, 0);
    NLA_PUT_U32(message, NL80211_ATTR_IFINDEX, self->ifindex);

    int err;
    struct nlattr *answer[NUM_NL80211_ATTR];
    if ( ( err = _j4status_nl_send_message(self->context, message, answer, NL80211_ATTR_MAX, NULL) ) != 0 )
    {
        if ( err == -NLE_NOADDR )
            ret = TRUE;
        else if ( err < 0 )
            g_warning("Couldn’t query nl80211 status for %s: %s", interface, nl_geterror(err));
        goto fail;
    }

    self->wifi.is = TRUE;

    _j4status_nl_section_update_nl80211(self);

nla_put_failure:
fail:
    nlmsg_free(message);
    return ret || self->wifi.is;
}

static GVariant *
_j4status_nl_section_get_addresses(const J4statusNlSection *self)
{
    GVariantBuilder builder;
    GList *address_;
    gchar address[44]; /* ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128 + \0 */

    g_variant_builder_init(&builder, G_VARIANT_TYPE_STRING_ARRAY);

    for ( address_ = self->addresses.ipv4 ; address_ != NULL ; address_ = g_list_next(address_) )
    {
        gchar *p;
        nl_addr2str(address_->data, address, sizeof(address));
        p = g_utf8_strchr(address, -1, '/');
        g_assert_nonnull(p); /* We know libnl wrote the prefix length */
        *p = '\0';
        g_variant_builder_add_value(&builder, g_variant_new_string(address));
    }

    if ( self->context->addresses != ADDRESSES_IPV4 )
    for ( address_ = self->addresses.ipv6 ; address_ != NULL ; address_ = g_list_next(address_) )
    {
        gchar *p;
        nl_addr2str(address_->data, address, sizeof(address));
        p = g_utf8_strchr(address, -1, '/');
        g_assert_nonnull(p); /* We know libnl wrote the prefix length */
        *p = '\0';
        g_variant_builder_add_value(&builder, g_variant_new_string(address));
    }

    return g_variant_builder_end(&builder);
}

static GVariant *
_j4status_nl_format_up_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    const J4statusNlSection *self = user_data;

    switch ( value )
    {
    case TOKEN_UP_ADDRESSES:
        return _j4status_nl_section_get_addresses(self);
    default:
        g_assert_not_reached();
    }
    return NULL;
}

static GVariant *
_j4status_nl_format_down_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    return NULL;
}

static GVariant *
_j4status_nl_format_up_wifi_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    const J4statusNlSection *self = user_data;

    switch ( value )
    {
    case TOKEN_UP_WIFI_ADDRESSES:
        return _j4status_nl_section_get_addresses(self);
    case TOKEN_UP_WIFI_STRENGTH:
        if ( self->wifi.strength < 0 )
            return NULL;
        return g_variant_new_byte(self->wifi.strength);
    case TOKEN_UP_WIFI_SSID:
        if ( self->wifi.ssid == NULL )
            return NULL;
        return g_variant_new_string(self->wifi.ssid);
    case TOKEN_UP_WIFI_BITRATE:
        if ( self->wifi.bitrate < 1 )
            return NULL;
        return g_variant_new_uint64(self->wifi.bitrate);
    }
    return NULL;
}

static GVariant *
_j4status_nl_format_down_wifi_callback(const gchar *token, guint64 value, gconstpointer user_data)
{
    const J4statusNlSection *self = user_data;

    switch ( value )
    {
    case TOKEN_DOWN_WIFI_APS:
        if ( self->wifi.aps < 0 )
            return NULL;
        return g_variant_new_uint64(self->wifi.aps);
    }
    return NULL;
}

static void
_j4status_nl_section_free_addresses(J4statusNlSection *self)
{
    g_list_free_full(self->addresses.ipv4, (GDestroyNotify) nl_addr_put);
    g_list_free_full(self->addresses.ipv6, (GDestroyNotify) nl_addr_put);
    self->addresses.ipv4 = NULL;
    self->addresses.ipv6 = NULL;
    self->addresses.has = FALSE;
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
    if ( ! ( flags & IFF_UP ) )
    {
        /* Unavailable */
        _j4status_nl_section_free_addresses(self);
    }
    else if ( ! ( flags & IFF_RUNNING ) )
    {
        state = J4STATUS_STATE_BAD;

        if ( self->wifi.is )
            value = j4status_format_string_replace(self->context->formats.down_wifi, _j4status_nl_format_down_wifi_callback, self);
        else
            value = j4status_format_string_replace(self->context->formats.down, _j4status_nl_format_down_callback, self);
        _j4status_nl_section_free_addresses(self);
    }
    else if ( ! self->addresses.has )
    {
        state = J4STATUS_STATE_AVERAGE;
        if ( self->wifi.is && self->wifi.has_ap )
        {
            if ( self->wifi.ssid == NULL )
                value = g_strdup("Associating");
            else
                value = g_strdup_printf("Associating with %s", self->wifi.ssid);
        }
        else
            value = g_strdup("Connecting");
    }
    else
    {
        state = J4STATUS_STATE_GOOD;

        if ( self->wifi.is )
            value = j4status_format_string_replace(self->context->formats.up_wifi, _j4status_nl_format_up_wifi_callback, self);
        else
            value = j4status_format_string_replace(self->context->formats.up, _j4status_nl_format_up_callback, self);
    }

    j4status_section_set_state(self->section, state);
    j4status_section_set_value(self->section, value);
}

static gint
_j4status_nl_address_compare(gconstpointer a, gconstpointer b)
{
    if ( a == b )
        return 0;
    return nl_addr_cmp(a, b);
}

static gboolean
_j4status_nl_section_add_address(J4statusNlSection *self, struct rtnl_addr *rtaddr)
{
    struct nl_addr *addr;
    addr = rtnl_addr_get_local(rtaddr);
    GList **list;
    gboolean add;
    switch ( nl_addr_get_family(addr) )
    {
    case AF_INET:
        list = &self->addresses.ipv4;
        add = ( self->context->addresses != ADDRESSES_IPV6 );
    break;
    case AF_INET6:
    {
        guint8 *bin;
        /* IPv6 addresses ar 128bit long, so 16 bytes */
        g_return_val_if_fail(nl_addr_get_len(addr) == 16, FALSE);
        bin = nl_addr_get_binary_addr(addr);
        /* We only keep Unicast routable addresses */
        if ( ( bin[0] & 0xE0 ) != 0x20 )
            return FALSE;
        list = &self->addresses.ipv6;
        add = ( self->context->addresses != ADDRESSES_IPV4 );
    }
    break;
    default:
        /* Not supported */
        return FALSE;
    }

    gboolean had = self->addresses.has;
    self->addresses.has = TRUE;

    gboolean need = ( ( ( ! self->wifi.is ) && ( self->context->formats.up_tokens & TOKEN_FLAG_UP_ADDRESSES ) )
                      || ( self->wifi.is && ( self->context->formats.up_wifi_tokens & TOKEN_FLAG_UP_WIFI_ADDRESSES) ) );
    if ( ! need )
        return ( ! had );

    if ( ( ! add ) || ( g_list_find_custom(*list, addr,_j4status_nl_address_compare) != NULL ) )
        /* Already got it */
        return FALSE;

    *list = g_list_prepend(*list, nl_addr_get(addr));
    return TRUE;
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

    _j4status_nl_section_check_nl80211(self, interface);

    struct nl_object *object;
    for ( object = nl_cache_get_first(context->addr_cache) ; object != NULL ; object = nl_cache_get_next(object) )
    {
        struct rtnl_addr *addr = nl_object_priv(object);
        if ( rtnl_addr_get_ifindex(addr) == self->ifindex )
            _j4status_nl_section_add_address(self, addr);
    }

    _j4status_nl_section_update(self);

    return self;
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

        if ( section->link != link )
        {
            _j4status_nl_section_free_addresses(section);
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

        if ( ! _j4status_nl_section_add_address(section, addr) )
            return;
    }
    else
        g_assert_not_reached();
    _j4status_nl_section_update(section);
}

static int
_j4status_nl_nl80211_no_seq_check(struct nl_msg *msg, void *user_data)
{
    return NL_OK;
}

static int
_j4status_nl_nl80211_event(struct nl_msg *msg, void *user_data)
{
    J4statusPluginContext *self = user_data;
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    J4statusNlSection *section;

    struct nlattr *answer[NUM_NL80211_ATTR];
    nla_parse(answer, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if ( answer[NL80211_ATTR_IFINDEX] == NULL )
        return NL_SKIP;

    section = g_hash_table_lookup(self->sections, GINT_TO_POINTER(nla_get_u32(answer[NL80211_ATTR_IFINDEX])));


    switch ( gnlh->cmd )
    {
    case NL80211_CMD_NEW_STATION:
    case NL80211_CMD_JOIN_IBSS:
    case NL80211_CMD_AUTHENTICATE:
    case NL80211_CMD_ASSOCIATE:
    case NL80211_CMD_CONNECT:
        _j4status_nl_section_update_nl80211(section);
    break;
    case NL80211_CMD_NEW_SCAN_RESULTS:
        _j4status_nl_section_update_nl80211(section);
    break;
    default:
    break;
    }

    return NL_SKIP;
}


static void _j4status_nl_uninit(J4statusPluginContext *self);

static J4statusPluginContext *
_j4status_nl_init(J4statusCoreInterface *core)
{
    gchar **interfaces = NULL;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("Netlink");
    if ( key_file != NULL )
    {
        interfaces = g_key_file_get_string_list(key_file, "Netlink", "Interfaces", NULL, NULL);

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

    self->sections = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, _j4status_nl_section_free);

    guint64 addresses = ADDRESSES_ALL;
    gchar *format_up = NULL;
    gchar *format_down = NULL;
    gchar *format_up_wifi = NULL;
    gchar *format_down_wifi = NULL;

    key_file = j4status_config_get_key_file("Netlink Formats");
    if ( key_file != NULL )
    {
        j4status_config_key_file_get_enum(key_file, "Netlink Formats", "Addresses", _j4status_nl_addresses, G_N_ELEMENTS(_j4status_nl_addresses), &addresses);
        format_up         = g_key_file_get_string(key_file, "Netlink Formats", "Up", NULL);
        format_down       = g_key_file_get_string(key_file, "Netlink Formats", "Down", NULL);
        format_up_wifi    = g_key_file_get_string(key_file, "Netlink Formats", "UpWiFi", NULL);
        format_down_wifi  = g_key_file_get_string(key_file, "Netlink Formats", "DownWiFi", NULL);

        g_key_file_free(key_file);
    }

    self->addresses = addresses;
    self->formats.up        = j4status_format_string_parse(format_up,        _j4status_nl_format_up_tokens,        G_N_ELEMENTS(_j4status_nl_format_up_tokens),        J4STATUS_NL_DEFAULT_FORMAT_UP,        &self->formats.up_tokens);
    self->formats.down      = j4status_format_string_parse(format_down,      NULL,                                 0,                                                  J4STATUS_NL_DEFAULT_FORMAT_DOWN,      NULL);
    self->formats.up_wifi   = j4status_format_string_parse(format_up_wifi,   _j4status_nl_format_up_wifi_tokens,   G_N_ELEMENTS(_j4status_nl_format_up_wifi_tokens),   J4STATUS_NL_DEFAULT_FORMAT_UP_WIFI,   &self->formats.up_wifi_tokens);
    self->formats.down_wifi = j4status_format_string_parse(format_down_wifi, _j4status_nl_format_down_wifi_tokens, G_N_ELEMENTS(_j4status_nl_format_down_wifi_tokens), J4STATUS_NL_DEFAULT_FORMAT_DOWN_WIFI, &self->formats.down_wifi_tokens);

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
        nl_cache_put(self->link_cache);
        goto error;
    }

    if ( ( self->formats.up_tokens & TOKEN_FLAG_UP_ADDRESSES ) || ( self->formats.up_wifi_tokens & TOKEN_FLAG_UP_WIFI_ADDRESSES ) )
    {
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
            nl_cache_put(self->addr_cache);
            goto error;
        }
    }

    if ( ( self->formats.up_wifi_tokens & ~TOKEN_FLAG_UP_WIFI_ADDRESSES ) || ( self->formats.down_wifi_tokens ) )
    {
        self->nl80211.source = g_water_nl_source_new_sock(NULL, NETLINK_GENERIC);
        if ( self->nl80211.source == NULL )
        {
            g_warning("Couldn't subscribe to nl80211 events");
            goto error;
        }
        self->nl80211.sock = g_water_nl_source_get_sock(self->nl80211.source);

        self->nl80211.id = genl_ctrl_resolve(self->nl80211.sock, NL80211_GENL_NAME);
        if ( self->nl80211.id < 0 )
        {
            g_warning("Couldn't resolve nl80211: %s", nl_geterror(self->nl80211.id));
            goto error;
        }

        nl_socket_modify_err_cb(self->nl80211.sock, NL_CB_CUSTOM, _j4status_nl_message_error_callback, &self->nl80211.data);
        nl_socket_modify_cb(self->nl80211.sock, NL_CB_ACK, NL_CB_CUSTOM, _j4status_nl_message_ack_callback, &self->nl80211.data);
        nl_socket_modify_cb(self->nl80211.sock, NL_CB_FINISH, NL_CB_CUSTOM, _j4status_nl_message_ack_callback, &self->nl80211.data);
        nl_socket_modify_cb(self->nl80211.sock, NL_CB_VALID, NL_CB_CUSTOM, _j4status_nl_message_valid_callback, &self->nl80211.data);

        self->nl80211.esource = g_water_nl_source_new_sock(NULL, NETLINK_GENERIC);
        if ( self->nl80211.esource == NULL )
        {
            g_warning("Couldn't subscribe to nl80211 events");
            goto error;
        }
        self->nl80211.esock = g_water_nl_source_get_sock(self->nl80211.esource);

        if ( ! _j4status_nl_register_events(self) )
            goto error;

        nl_socket_modify_cb(self->nl80211.esock, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, _j4status_nl_nl80211_no_seq_check, self);
        nl_socket_modify_cb(self->nl80211.esock, NL_CB_VALID, NL_CB_CUSTOM, _j4status_nl_nl80211_event, self);
    }

    gchar **interface;
    for ( interface = interfaces ; *interface != NULL ; ++interface )
    {
        J4statusNlSection *section;
        section = _j4status_nl_section_new(self, core, *interface);
        if ( section != NULL )
            g_hash_table_insert(self->sections, GINT_TO_POINTER(section->ifindex), section);
        g_free(*interface);
    }

    g_free(interfaces);


    if ( g_hash_table_size(self->sections) < 1 )
        goto error;


    return self;

error:
    _j4status_nl_uninit(self);
    return NULL;
}

static void
_j4status_nl_uninit(J4statusPluginContext *self)
{
    j4status_format_string_unref(self->formats.down_wifi);
    j4status_format_string_unref(self->formats.up_wifi);
    j4status_format_string_unref(self->formats.down);
    j4status_format_string_unref(self->formats.up);

    if ( self->nl80211.esource != NULL )
        g_water_nl_source_free(self->nl80211.esource);

    if ( self->nl80211.source != NULL )
        g_water_nl_source_free(self->nl80211.source);

    if ( self->source != NULL )
        g_water_nl_source_free(self->source);

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

J4STATUS_EXPORT void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_nl_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_nl_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_nl_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_nl_stop);
}
