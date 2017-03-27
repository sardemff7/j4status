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

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-output.h>

#define BOOL_TO_S(bool) ((bool) ? "yes" : "no")

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    gsize last_len;
    GString *line;
};

struct _J4statusOutputPluginStream {
    J4statusPluginContext *context;
    J4statusCoreStream *stream;
    GDataOutputStream *out;
};

static J4statusPluginContext *
_j4status_debug_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->line = g_string_new("");

    return context;
}

static void
_j4status_debug_uninit(J4statusPluginContext *context)
{
    g_string_free(context->line, TRUE);

    g_free(context);
}

static J4statusOutputPluginStream *
_j4status_debug_stream_new(J4statusPluginContext *context, J4statusCoreStream *core_stream)
{
    J4statusOutputPluginStream *stream;

    stream = g_slice_new0(J4statusOutputPluginStream);
    stream->context = context;
    stream->stream = core_stream;

    stream->out = g_data_output_stream_new(j4status_core_stream_get_output_stream(stream->context->core, stream->stream));

    return stream;
}

static void
_j4status_debug_stream_free(J4statusPluginContext *context, J4statusOutputPluginStream *stream)
{
    g_object_unref(stream->out);

    g_slice_free(J4statusOutputPluginStream, stream);
}

static void
_j4status_debug_generate_line(J4statusPluginContext *context, GList *sections)
{
    g_string_truncate(context->line, 0);
    gboolean first = TRUE;
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;

        if ( ! j4status_section_is_dirty(section) )
            goto print;

        const gchar *align = NULL;
        switch ( j4status_section_get_align(section) )
        {
        case J4STATUS_ALIGN_LEFT:
            align = "left";
        break;
        case J4STATUS_ALIGN_RIGHT:
            align = "right";
        break;
        case J4STATUS_ALIGN_CENTER:
            align = "center";
        break;
        }

        J4statusState state = j4status_section_get_state(section);
        const gchar *state_s = NULL;
        switch ( state & ~J4STATUS_STATE_FLAGS )
        {
        case J4STATUS_STATE_NO_STATE:
            state_s = "no state";
        break;
        case J4STATUS_STATE_UNAVAILABLE:
            state_s = "unavailable";
        break;
        case J4STATUS_STATE_BAD:
            state_s = "bad";
        break;
        case J4STATUS_STATE_AVERAGE:
            state_s = "average";
        break;
        case J4STATUS_STATE_GOOD:
            state_s = "good";
        break;
        }

#define set_colour(n) \
    gchar n##_str[10] = "none"; \
    G_STMT_START { \
        const gchar *tmp = j4status_colour_to_hex(j4status_section_get_##n(section)); \
        if ( tmp != NULL ) \
            g_snprintf(n##_str, sizeof(n##_str), "%s", tmp); \
    } G_STMT_END
        set_colour(label_colour);
        set_colour(colour);
#undef set_colour

        gchar *cache;
        cache = g_strdup_printf("--"
            "\nName: %s"
            "\nInstance: %s"
            "\nLabel: %s"
            "\nLabel colour: %s"
            "\nAlignment: %s"
            "\nMaximum width: %jd"
            "\n"
            "\nState: %s"
            "\nUrgent: %s"
            "\nColour: %s"
            "\nValue: %s"
            "\n--",
            j4status_section_get_name(section),
            j4status_section_get_instance(section),
            j4status_section_get_label(section),
            label_colour_str,
            align,
            j4status_section_get_max_width(section),
            state_s,
            BOOL_TO_S(state & J4STATUS_STATE_URGENT),
            colour_str,
            j4status_section_get_value(section));

        j4status_section_set_cache(section, cache);

    print:
        g_string_append(context->line, j4status_section_get_cache(section));
        if ( first )
            first = FALSE;
        else
            g_string_append_c(context->line, '\n');
    }
}

static gboolean
_j4status_debug_send_line(J4statusPluginContext *context, J4statusOutputPluginStream *stream, GError **error)
{
    return g_data_output_stream_put_string(stream->out, context->line->str, NULL, error);
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_init_callback(interface, _j4status_debug_init);
    libj4status_output_plugin_interface_add_uninit_callback(interface, _j4status_debug_uninit);

    libj4status_output_plugin_interface_add_stream_new_callback(interface, _j4status_debug_stream_new);
    libj4status_output_plugin_interface_add_stream_free_callback(interface, _j4status_debug_stream_free);

    libj4status_output_plugin_interface_add_generate_line_callback(interface, _j4status_debug_generate_line);
    libj4status_output_plugin_interface_add_send_line_callback(interface, _j4status_debug_send_line);
}
