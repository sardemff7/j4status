/*
 * j4status - Status line generator
 *
 * Copyright © 2012-2018 Quentin "Sardem FF7" Glidic
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

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#ifdef G_OS_UNIX
#include <gio/gunixsocketaddress.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixinputstream.h>
#endif /* G_OS_UNIX */

#ifdef ENABLE_SYSTEMD
#include <sys/socket.h>
#include <systemd/sd-daemon.h>
#endif /* ENABLE_SYSTEMD */

#include "j4status.h"
#include "j4status-plugin.h"
#include "j4status-plugin-output.h"
#include "j4status-plugin-input.h"
#include "j4status-plugin-private.h"
#include "plugins.h"

#include "io.h"

struct _J4statusIOContext {
    J4statusCoreContext *core;
    J4statusOutputPlugin *plugin;
    GSocketService *server;
    GList *streams;
    GList *paths_to_unlink;
};

struct _J4statusIOStream {
    J4statusIOContext *io;
    guint tries;
    GSocketAddress *address;
    GSocketConnection *connection;
    GInputStream *in;
    GOutputStream *out;
    J4statusOutputPluginStream *stream;
    gboolean header_sent;
};

#define MAX_TRIES 3

static void _j4status_io_stream_connect_callback(GObject *obj, GAsyncResult *res, gpointer user_data);
static void _j4status_io_stream_put_header(J4statusIOStream *stream);

static void
_j4status_io_stream_cleanup(J4statusIOStream *self)
{
    if ( self->in != NULL )
    {
        g_object_unref(self->in);
        self->in = NULL;
    }
    if ( self->out != NULL )
    {
        g_object_unref(self->out);
        self->out = NULL;
    }

    if ( self->connection != NULL )
    {
        g_object_unref(self->connection);
        self->connection = NULL;
    }

    self->io->plugin->interface.stream_free(self->io->plugin->context, self->stream);
    self->stream = NULL;
}

static void
_j4status_io_stream_set_connection(J4statusIOStream *self, GSocketConnection *connection)
{
    self->connection = connection;
    self->in = g_object_ref(g_io_stream_get_input_stream(G_IO_STREAM(self->connection)));
    self->out = g_object_ref(g_io_stream_get_output_stream(G_IO_STREAM(self->connection)));
    self->stream = self->io->plugin->interface.stream_new(self->io->plugin->context, self);
    if ( ! self->header_sent )
        _j4status_io_stream_put_header(self);
}

static void
_j4status_io_stream_connect(J4statusIOStream *self)
{
    _j4status_io_stream_cleanup(self);

    GSocketClient *client;
    client = g_socket_client_new();

    g_socket_client_connect_async(client, G_SOCKET_CONNECTABLE(self->address), NULL, _j4status_io_stream_connect_callback, self);
    g_object_unref(client);
}

GInputStream *
j4status_io_stream_get_input_stream(J4statusIOStream *self)
{
    return self->in;
}

GOutputStream *
j4status_io_stream_get_output_stream(J4statusIOStream *self)
{
    return self->out;
}

void
j4status_io_stream_reconnect(J4statusIOStream *self)
{
    if ( self->connection == NULL )
        /* Not a socket stream */
        return;

    if ( self->address != NULL )
        /* Client stream */
        _j4status_io_stream_connect(self);
    else
        /* Server stream */
        j4status_io_stream_free(self);
}

static void
_j4status_io_stream_connect_callback(GObject *obj, GAsyncResult *res, gpointer user_data)
{
    J4statusIOStream *self = user_data;

    GError *error = NULL;
    GSocketConnection *connection;
    connection = g_socket_client_connect_finish(G_SOCKET_CLIENT(obj), res, &error);

    if ( connection == NULL )
    {
        g_warning("Couldn't reconnect: %s", error->message);
        g_clear_error(&error);
        if ( ++self->tries > MAX_TRIES )
            j4status_io_stream_free(self);
        else
            _j4status_io_stream_connect(self);
    }
    else
        _j4status_io_stream_set_connection(self, connection);
}

static J4statusIOStream *
_j4status_io_stream_new(J4statusIOContext *io)
{
    J4statusIOStream *self;
    self = g_slice_new0(J4statusIOStream);
    self->io = io;

    return self;
}

static J4statusIOStream *
_j4status_io_stream_new_for_connection(J4statusIOContext *io, GSocketConnection *connection)
{
    J4statusIOStream *self = _j4status_io_stream_new(io);
    _j4status_io_stream_set_connection(self, g_object_ref(connection));

    return self;
}

static void
_j4status_io_stream_add(J4statusIOContext *self, const gchar *stream_desc)
{
    J4statusIOStream *stream;
    if ( g_strcmp0(stream_desc, "std") == 0 )
    {
        GOutputStream *out;
        GInputStream *in;
#ifdef G_OS_UNIX
        out = g_unix_output_stream_new(1, FALSE);
        in = g_unix_input_stream_new(0, FALSE);
#else /* ! G_OS_UNIX */
        return;
#endif /* ! G_OS_UNIX */

        stream = _j4status_io_stream_new(self);

        stream->out = out;
        stream->in = in;

        stream->stream = stream->io->plugin->interface.stream_new(stream->io->plugin->context, stream);

        _j4status_io_stream_put_header(stream);

        goto end;
    }

    GSocketAddress *address = NULL;

    if ( g_str_has_prefix(stream_desc, "tcp:") )
    {
        const gchar *uri = stream_desc + strlen("tcp:");
        gchar *port_str = g_utf8_strrchr(uri, -1, ':');
        if ( port_str == NULL )
            /* No port, illegal stream description */
            return;

        *port_str = '\0';
        ++port_str;

        guint64 port;
        port = g_ascii_strtoull(port_str, NULL, 10);
        if ( port > 65535 )
            return;

        GInetAddress *inet_address;
        inet_address = g_inet_address_new_from_string(uri);

        address = g_inet_socket_address_new(inet_address, port);
    }

#ifdef G_OS_UNIX
    if ( g_str_has_prefix(stream_desc, "unix:") )
    {
        const gchar *path = stream_desc + strlen("unix:");

        address = g_unix_socket_address_new(path);
    }
#endif /* G_OS_UNIX */

    if ( address == NULL )
        return;

    stream = _j4status_io_stream_new(self);
    stream->address = address;

    _j4status_io_stream_connect(stream);

end:
    self->streams = g_list_prepend(self->streams, stream);
}

static void
_j4status_io_stream_free(gpointer data)
{
    J4statusIOStream *self = data;

    _j4status_io_stream_cleanup(self);

    if ( self->connection != NULL )
        g_object_unref(self->connection);
    if ( self->address != NULL )
        g_object_unref(self->address);

    g_slice_free(J4statusIOStream, self);
}

static gboolean
_j4status_io_stream_put_string(J4statusIOStream *self, J4statusPluginSendFunc send_func)
{
    if ( send_func == NULL )
        return TRUE;
    if ( self->out == NULL )
        return FALSE;

    GError *error = NULL;
    if ( send_func(self->io->plugin->context, self->stream, &error) )
        return TRUE;

    /*
     * We do not output on broken pipe
     * because this is what we get on disconnect.
     * Too frequent to warrant a warning.
     */
    if ( ! g_error_matches(error, G_IO_ERROR, G_IO_ERROR_BROKEN_PIPE) )
        g_warning("Couldn't write line: %s", error->message);
    g_clear_error(&error);

    j4status_io_stream_reconnect(self);

    return FALSE;
}

static void
_j4status_io_stream_put_header(J4statusIOStream *self)
{
    self->header_sent = _j4status_io_stream_put_string(self, self->io->plugin->interface.send_header);
}

static void
_j4status_io_stream_put_line(J4statusIOStream *self)
{
    if ( self->header_sent )
        _j4status_io_stream_put_string(self, self->io->plugin->interface.send_line);
}

static gboolean
_j4status_io_server_callback(GSocketService *service, GSocketConnection *connection, GObject *source_object, gpointer user_data)
{
    J4statusIOContext *self = user_data;
    J4statusIOStream *stream;

    stream = _j4status_io_stream_new_for_connection(self, connection);
    self->streams = g_list_prepend(self->streams, stream);
    _j4status_io_stream_put_line(stream);

    return FALSE;
}


static gboolean
_j4status_io_add_server(J4statusIOContext *self)
{
    if ( self->server != NULL )
        return FALSE;

    self->server = g_socket_service_new();
    g_signal_connect(self->server, "incoming", (GCallback) _j4status_io_server_callback, self);

    return TRUE;
}

static void
_j4status_io_add_systemd(J4statusIOContext *self)
{
#ifdef ENABLE_SYSTEMD
    gint fds = sd_listen_fds(TRUE);
    if ( fds < 0 )
    {
        g_warning("Failed to acquire systemd sockets: %s", g_strerror(-fds));
        return;
    }
    if ( fds == 0 )
        return;

    gboolean socket_added = FALSE;
    _j4status_io_add_server(self);

    GError *error = NULL;
    gint fd;
    for ( fd = SD_LISTEN_FDS_START ; fd < SD_LISTEN_FDS_START + fds ; ++fd )
    {
        gint r;
        r = sd_is_socket(fd, AF_UNSPEC, SOCK_STREAM, 1);
        if ( r < 0 )
        {
            g_warning("Failed to verify systemd socket type: %s", g_strerror(-r));
            continue;
        }

        if ( r == 0 )
            continue;

        GSocket *socket;
        socket = g_socket_new_from_fd(fd, &error);
        if ( socket == NULL )
        {
            g_warning("Failed to take a socket from systemd: %s", error->message);
            g_clear_error(&error);
            continue;
        }

        if ( ! g_socket_listener_add_socket(G_SOCKET_LISTENER(self->server), socket, NULL, &error) )
        {
            g_warning("Failed to add systemd socket to server: %s", error->message);
            g_clear_error(&error);
            continue;
        }

        socket_added = TRUE;
    }

    if ( ! socket_added )
    {
        g_object_unref(self->server);
        self->server = NULL;
    }
#endif /* ENABLE_SYSTEMD */
}

static void
_j4status_io_server_add(J4statusIOContext *self, const gchar *server_desc)
{
    GSocketAddress *address = NULL;
    const gchar *path = NULL;

    if ( g_str_has_prefix(server_desc, "tcp:") )
    {
        GInetAddress *inet_address;
        const gchar *uri = server_desc + strlen("tcp:");
        gchar *port_str = g_utf8_strrchr(uri, -1, ':');
        if ( port_str == NULL )
        {
            /* No host, only port */
            port_str = (gchar *) uri;
            /* If you want IPv4, just use "0.0.0.0" */
            inet_address = g_inet_address_new_any(G_SOCKET_FAMILY_IPV6);
        }
        else
        {
            *port_str = '\0';
            ++port_str;
            inet_address = g_inet_address_new_from_string(uri);
        }


        guint64 port;
        port = g_ascii_strtoull(port_str, NULL, 10);
        if ( port > 65535 )
            return;


        address = g_inet_socket_address_new(inet_address, port);
    }

#ifdef G_OS_UNIX
    if ( g_str_has_prefix(server_desc, "unix:") )
    {
        path = server_desc + strlen("unix:");

        address = g_unix_socket_address_new(path);
    }
#endif /* G_OS_UNIX */

    if ( address == NULL )
        return;

    gboolean need_free_server = _j4status_io_add_server(self);

    GError *error = NULL;
    gboolean r;
    r = g_socket_listener_add_address(G_SOCKET_LISTENER(self->server), address, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL, NULL, &error);
    g_object_unref(address);

    if ( r )
    {
        if ( path != NULL )
            self->paths_to_unlink = g_list_prepend(self->paths_to_unlink, g_strdup(path));
        return;
    }

    g_warning("Couldn't add listener for '%s': %s", server_desc, error->message);
    g_clear_error(&error);

    if ( need_free_server )
    {
        g_object_unref(self->server);
        self->server = NULL;
    }
}

static inline gboolean
_j4status_io_has_stream(J4statusIOContext *self)
{
    return ( ( self->streams != NULL ) || ( self->server != NULL ) );
}

J4statusIOContext *
j4status_io_new(J4statusCoreContext *core, J4statusOutputPlugin *plugin, const gchar * const *servers_desc, const gchar * const *streams_desc)
{
    J4statusIOContext *self;
    self = g_new0(J4statusIOContext, 1);
    self->core = core;
    self->plugin = plugin;

    _j4status_io_add_systemd(self);

    if ( ( servers_desc == NULL ) && ( streams_desc == NULL ) && ( ! _j4status_io_has_stream(self) ) )
        /* Using stdin/stdout */
        _j4status_io_stream_add(self, "std");

    if ( servers_desc != NULL )
    {
        const gchar * const *server_desc;
        for ( server_desc = servers_desc ; *servers_desc != NULL ; ++servers_desc)
            _j4status_io_server_add(self, *server_desc);
    }

    if ( streams_desc != NULL )
    {
        const gchar * const *stream_desc;
        for ( stream_desc = streams_desc ; *stream_desc != NULL ; ++stream_desc)
            _j4status_io_stream_add(self, *stream_desc);
    }

    if ( _j4status_io_has_stream(self) )
        return self;

    j4status_io_free(self);
    return NULL;
}

static void
_j4status_io_unlink_path(gpointer data)
{
    gchar *path = data;
    g_unlink(path);
    g_free(path);
}

void
j4status_io_free(J4statusIOContext *self)
{
    g_list_free_full(self->streams, _j4status_io_stream_free);
    g_list_free_full(self->paths_to_unlink, _j4status_io_unlink_path);

    if ( self->server != NULL )
        g_object_unref(self->server);

    g_free(self);
}

void
j4status_io_stream_free(J4statusIOStream *self)
{
    J4statusIOContext *io = self->io;
    io->streams = g_list_remove(io->streams, self);
    _j4status_io_stream_free(self);
    if ( ! _j4status_io_has_stream(io) )
        j4status_core_quit(io->core);
}

void
j4status_io_update_line(J4statusIOContext *self)
{
    GList *stream = self->streams;
    while ( stream != NULL )
    {
        GList *next = g_list_next(stream);
        _j4status_io_stream_put_line(stream->data);
        stream = next;
    }
}
