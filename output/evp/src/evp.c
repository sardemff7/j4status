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
#include <glib/gprintf.h>

#include <libeventd-event.h>
#include <libeventc.h>

#include "j4status-plugin-output.h"

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    J4statusColour colours[_J4STATUS_STATE_SIZE];
    gboolean align;
    EventcConnection *eventc;
};

struct _J4statusOutputPluginStream {
    J4statusPluginContext *context;
    J4statusCoreStream *stream;
    GDataInputStream *in;
    GOutputStream *out;
};

static J4statusOutputPluginStream *
_j4status_evp_stream_new(G_GNUC_UNUSED J4statusPluginContext *context, G_GNUC_UNUSED J4statusCoreStream *core_stream)
{
    return NULL;
}

static void
_j4status_evp_stream_free(G_GNUC_UNUSED J4statusPluginContext *context, G_GNUC_UNUSED J4statusOutputPluginStream *stream)
{
}

static void
_j4status_evp_generate_line(J4statusPluginContext *context, GList *sections)
{
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;
        if ( ! j4status_section_is_dirty(section) )
            continue;

        const gchar *value;
        value = j4status_section_get_value(section);
        if ( value == NULL )
            continue;

        J4statusColour colour = {0};

        EventdEvent *event;
        event = j4status_section_get_output_user_data(section);
        if ( event == NULL )
        {
            event = eventd_event_new("j4status", j4status_section_get_name(section));
            j4status_section_set_output_user_data(section, event, (GDestroyNotify) eventd_event_unref);

            eventd_event_add_data_string(event, g_strdup("instance"), g_strdup(j4status_section_get_instance(section)));

            const gchar *label;
            label = j4status_section_get_label(section);
            if ( label != NULL )
            {
                eventd_event_add_data_string(event, g_strdup("label"), g_strdup(label));
                colour = j4status_section_get_label_colour(section);
                if ( colour.set )
                    eventd_event_add_data_string(event, g_strdup("label-colour"), g_strdup(j4status_colour_to_hex(colour)));
            }
        }

        GVariantBuilder builder;
        const gchar *s = value, *c ;
        g_variant_builder_init(&builder, G_VARIANT_TYPE_STRING_ARRAY);
        while ( ( c = g_utf8_strchr(s, -1, '') ) != NULL )
        {
            g_variant_builder_add_value(&builder, g_variant_new_take_string(g_strndup(s, c - s)));
            s = g_utf8_next_char(c);
        }
        g_variant_builder_add_value(&builder, g_variant_new_string(s));

        eventd_event_add_data(event, g_strdup("value"), g_variant_builder_end(&builder));

        J4statusState state = j4status_section_get_state(section);
        colour = j4status_section_get_colour(section);
        if ( colour.set )
            eventd_event_add_data_string(event, g_strdup("colour"), g_strdup(j4status_colour_to_hex(colour)));
        colour = context->colours[state & ~J4STATUS_STATE_FLAGS];
        if ( colour.set )
            eventd_event_add_data_string(event, g_strdup("state-colour"), g_strdup(j4status_colour_to_hex(colour)));

        eventd_event_add_data(event, g_strdup("urgent"), g_variant_new_boolean(state & J4STATUS_STATE_URGENT));

        eventc_connection_send_event(context->eventc, event, NULL);

        j4status_section_set_cache(section, NULL);
    }
}

static void
_j4status_evp_update_colour(J4statusColour *colour, GKeyFile *key_file, gchar *name)
{
    gchar *config;
    config = g_key_file_get_string(key_file, "EvP", name, NULL);
    if ( config == NULL )
        return;

    *colour = j4status_colour_parse(config);
    g_free(config);
}

static void
_j4status_evp_connect_cb(G_GNUC_UNUSED GObject *obj, GAsyncResult *res, gpointer user_data)
{
    J4statusPluginContext *context = user_data;

    eventc_connection_connect_finish(context->eventc, res, NULL);
}

static J4statusPluginContext *
_j4status_evp_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->colours[J4STATUS_STATE_UNAVAILABLE].set = TRUE;
    context->colours[J4STATUS_STATE_UNAVAILABLE].blue = 0xff;
    context->colours[J4STATUS_STATE_BAD].set = TRUE;
    context->colours[J4STATUS_STATE_BAD].red = 0xff;
    context->colours[J4STATUS_STATE_AVERAGE].set = TRUE;
    context->colours[J4STATUS_STATE_AVERAGE].red = 0xff;
    context->colours[J4STATUS_STATE_AVERAGE].green = 0xff;
    context->colours[J4STATUS_STATE_GOOD].set = TRUE;
    context->colours[J4STATUS_STATE_GOOD].green = 0xff;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("EvP");

    if ( key_file != NULL )
    {
        context->align = g_key_file_get_boolean(key_file, "EvP", "Align", NULL);
        _j4status_evp_update_colour(&context->colours[J4STATUS_STATE_NO_STATE], key_file, "NoStateColour");
        _j4status_evp_update_colour(&context->colours[J4STATUS_STATE_UNAVAILABLE], key_file, "UnavailableColour");
        _j4status_evp_update_colour(&context->colours[J4STATUS_STATE_BAD], key_file, "BadColour");
        _j4status_evp_update_colour(&context->colours[J4STATUS_STATE_AVERAGE], key_file, "AverageColour");
        _j4status_evp_update_colour(&context->colours[J4STATUS_STATE_GOOD], key_file, "GoodColour");
    }

    if ( key_file != NULL )
        g_key_file_free(key_file);

    GError *error = NULL;

    context->eventc = eventc_connection_new(NULL, &error);
    if ( context->eventc == NULL )
        return NULL;

    eventc_connection_connect(context->eventc, _j4status_evp_connect_cb, context);

    return context;
}

static void
_j4status_evp_uninit(J4statusPluginContext *context)
{
    eventc_connection_close(context->eventc, NULL);
    g_object_unref(context->eventc);

    g_free(context);
}

J4STATUS_EXPORT void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_init_callback(interface, _j4status_evp_init);
    libj4status_output_plugin_interface_add_uninit_callback(interface, _j4status_evp_uninit);

    libj4status_output_plugin_interface_add_stream_new_callback(interface, _j4status_evp_stream_new);
    libj4status_output_plugin_interface_add_stream_free_callback(interface, _j4status_evp_stream_free);

    libj4status_output_plugin_interface_add_generate_line_callback(interface, _j4status_evp_generate_line);
}
