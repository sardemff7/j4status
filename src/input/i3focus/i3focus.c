
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <glib.h>
#include <glib-object.h>

#include <j4status-plugin-input.h>
#include <libj4status-config.h>

#include <i3ipc-glib/i3ipc-glib.h>

struct _J4statusPluginContext {
    i3ipcConnection *connection;
    J4statusSection *section;
};


static void
_j4status_i3focus_window_callback(GObject *object, i3ipcWindowEvent *event, gpointer user_data)
{
    J4statusSection *section = user_data;
    if ( g_strcmp0(event->change, "focus") != 0 )
        return;
    gchar *name;
    g_object_get(G_OBJECT(event->container), "name", &name, NULL);
    j4status_section_set_value(section, name);
}

static void _j4status_i3focus_uninit(J4statusPluginContext *context);

static J4statusPluginContext *
_j4status_i3focus_init(J4statusCoreInterface *core)
{
    i3ipcConnection *connection;
    i3ipcCommandReply *reply;
    GError *error = NULL;

    connection = i3ipc_connection_new(NULL, &error);
    if ( connection == NULL )
    {
        g_warning("Couldn't connection to i3: %s", error->message);
        g_clear_error(&error);
        return NULL;
    }

    reply = i3ipc_connection_subscribe(connection, I3IPC_EVENT_WINDOW, &error);
    if ( reply == NULL )
    {
        g_warning("Couldn't subscribe to i3 events: %s", error->message);
        g_clear_error(&error);
        g_object_unref(connection);
        return NULL;
    }
    if ( ! reply->success )
    {
        g_warning("Couldn't subscribe to i3 events: %s", reply->error);
        i3ipc_command_reply_free(reply);
        g_object_unref(connection);
        return NULL;
    }
    i3ipc_command_reply_free(reply);

    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->connection = connection;

    context->section = j4status_section_new(core);
    j4status_section_set_name(context->section, "i3focus");
    if ( ! j4status_section_insert(context->section) )
    {
        _j4status_i3focus_uninit(context);
        return NULL;
    }

    g_signal_connect(context->connection, "window", G_CALLBACK(_j4status_i3focus_window_callback), context->section);

    return context;
}

static void
_j4status_i3focus_uninit(J4statusPluginContext *context)
{
    j4status_section_free(context->section);

    g_object_unref(context->connection);

    g_free(context);
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_i3focus_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_i3focus_uninit);
}
