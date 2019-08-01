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
#include <glib-object.h>

#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>

#include "j4status-plugin-output.h"

#define yajl_strcmp(str1, len1, str2) ( ( strlen(str2) == len1 ) && ( g_ascii_strncasecmp((const gchar *) str1, str2, len1) == 0 ) )

typedef enum {
    KEY_NONE = 0,
    KEY_UNKNOWN,
    KEY_NAME,
    KEY_INSTANCE,
    KEY_BUTTON,
    KEY_X,
    KEY_Y,
    KEY_RELATIVE_X,
    KEY_RELATIVE_Y,
    KEY_WIDTH,
    KEY_HEIGHT,
} J4statusI3barOutputClickEventsJsonKey;

static gchar *_j4status_i3bar_output_json_key_names[] = {
    [KEY_NONE] = "none",
    [KEY_UNKNOWN] = "unknown",

    [KEY_NAME] = "name",
    [KEY_INSTANCE] = "instance",
    [KEY_BUTTON] = "button",
    [KEY_X] = "x",
    [KEY_Y] = "y",
    [KEY_RELATIVE_X] = "relative_x",
    [KEY_RELATIVE_Y] = "relative_y",
    [KEY_WIDTH] = "width",
    [KEY_HEIGHT] = "height",
};

typedef struct {
    gchar *error;
    guint array_nesting;
    guint array_key;
    gboolean in_event;
    J4statusI3barOutputClickEventsJsonKey key;
    gchar *name;
    gchar *instance;
    gchar *full_text;
    gint64 button;
} J4statusI3barOutputClickEventsParseContext;

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    struct {
        gchar *no_state;
        gchar *unavailable;
        gchar *bad;
        gchar *average;
        gchar *good;
    } colours;
    gboolean align;
    gsize last_len;
    gboolean no_click_events;
    yajl_handle json_handle;
    J4statusI3barOutputClickEventsParseContext parse_context;
    gchar *header;
    gchar *line;
};

struct _J4statusOutputPluginStream {
    J4statusPluginContext *context;
    J4statusCoreStream *stream;
    GDataInputStream *in;
    GDataOutputStream *out;
};

static int
_j4status_i3bar_output_click_events_integer(void *user_data, long long value)
{
    J4statusPluginContext *context = user_data;

    if ( ! context->parse_context.in_event )
    {
        if ( context->parse_context.key > KEY_UNKNOWN )
            context->parse_context.error = g_strdup_printf("Key '%s' must be in a section",
                _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        else
            context->parse_context.error = g_strdup_printf("Unexpected integer value: %lld", value);
        return 0;
    }

    switch ( context->parse_context.key )
    {
    case KEY_NONE:
        g_return_val_if_reached(0);
    case KEY_UNKNOWN:
        /* For forward compatibility, we ignore unknown keys */
    break;
    case KEY_X:
    case KEY_Y:
    case KEY_RELATIVE_X:
    case KEY_RELATIVE_Y:
    case KEY_WIDTH:
    case KEY_HEIGHT:
        /* Ignoring */
    break;
    case KEY_BUTTON:
        context->parse_context.button = value;
    break;
    default:
        context->parse_context.error = g_strdup_printf("Wrong integer key '%s'",
            _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_output_click_events_string(void *user_data, const unsigned char *value, size_t length)
{
    J4statusPluginContext *context = user_data;

    if ( ! context->parse_context.in_event )
    {
        if ( context->parse_context.key != KEY_NONE )
            context->parse_context.error = g_strdup_printf("Key '%s' must be in a section",
                _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        else
            context->parse_context.error = g_strdup_printf("Unexpected string value: %.*s", (gint) length, value);
        return 0;
    }

    switch ( context->parse_context.key )
    {
    case KEY_NONE:
        g_return_val_if_reached(0);
    case KEY_UNKNOWN:
        /* For forward compatibility, we ignore unknown keys */
    break;
    case KEY_NAME:
        context->parse_context.name = g_strndup((const gchar *) value, length);
    break;
    case KEY_INSTANCE:
        context->parse_context.instance = g_strndup((const gchar *) value, length);
    break;
    default:
        context->parse_context.error = g_strdup_printf("Wrong string key '%s'",
            _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_output_click_events_start_map(void *user_data)
{
    J4statusPluginContext *context = user_data;

    if ( context->parse_context.array_nesting < 1 )
    {
        context->parse_context.error = g_strdup_printf("Unexpected map");
        return 0;
    }

    if ( context->parse_context.in_event )
    {
        context->parse_context.error = g_strdup_printf("Unexpected map in section");
        return 0;
    }

    context->parse_context.in_event = TRUE;

    return 1;
}

static int
_j4status_i3bar_output_click_events_map_key(void *user_data, const unsigned char *value, size_t length)
{
    J4statusPluginContext *context = user_data;

    if ( ! context->parse_context.in_event )
    {
        context->parse_context.error = g_strdup_printf("Unexpected map key outside section: %.*s", (gint) length, value);
        return 0;
    }

    /* For forward compatibility, we ignore unknown keys */
    context->parse_context.key = KEY_UNKNOWN;

    gsize key;
    for ( key = KEY_UNKNOWN + 1 ; key < G_N_ELEMENTS(_j4status_i3bar_output_json_key_names) ; ++key )
    {
        if ( yajl_strcmp(value, length, _j4status_i3bar_output_json_key_names[key]) )
        {
            context->parse_context.key = key;
            break;
        }
    }

    return 1;
}

static int
_j4status_i3bar_output_click_events_end_map(void *user_data)
{
    J4statusPluginContext *context = user_data;

    if ( ! context->parse_context.in_event )
        return 0;

    const gchar *name = context->parse_context.name;
    const gchar *instance = context->parse_context.instance;
    if ( name == NULL )
    {
        if ( instance != NULL )
            context->parse_context.error = g_strdup_printf("Section instance but without name: %s", instance);
        else
            context->parse_context.error = g_strdup_printf("No section name to match the section to send the action to");
        return 0;
    }

    gchar *section_id, *event_id;
    if ( instance != NULL )
        section_id = g_strdup_printf("%s:%s", name, instance);
    else
        section_id = g_strdup(name);
    event_id = g_strdup_printf("mouse:%jd", context->parse_context.button);

    j4status_core_trigger_action(context->core, section_id, event_id);

    g_free(event_id);
    g_free(section_id);

    context->parse_context.in_event = FALSE;

    context->parse_context.key = KEY_NONE;

    g_free(context->parse_context.name);
    context->parse_context.name = NULL;
    g_free(context->parse_context.instance);
    context->parse_context.instance = NULL;

    return 1;
}

static int
_j4status_i3bar_output_click_events_start_array(void *user_data)
{
    J4statusPluginContext *context = user_data;

    switch ( context->parse_context.key )
    {
    case KEY_NONE:
        if ( ++context->parse_context.array_nesting > 1 )
        {
            context->parse_context.error = g_strdup_printf("Too much nested arrays: %u", context->parse_context.array_nesting);
            return 0;
        }
    break;
    case KEY_UNKNOWN:
        /* For forward compatibility, we ignore unknown keys */
        ++context->parse_context.array_key;
    break;
    default:
        context->parse_context.error = g_strdup_printf("Wrong array key '%s'",
            _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        return 0;
    }

    return 1;
}

static int
_j4status_i3bar_output_click_events_end_array(void *user_data)
{
    J4statusPluginContext *context = user_data;

    switch ( context->parse_context.key )
    {
    case KEY_NONE:
        --context->parse_context.array_nesting;
    break;
    case KEY_UNKNOWN:
        /* For forward compatibility, we ignore unknown keys */
        --context->parse_context.array_key;
    break;
    default:
        context->parse_context.error = g_strdup_printf("Wrong array key '%s'",
            _j4status_i3bar_output_json_key_names[context->parse_context.key]);
        return 0;
    }

    return 1;
}


static yajl_callbacks _j4status_i3bar_output_click_events_callbacks = {
    .yajl_integer     = _j4status_i3bar_output_click_events_integer,
    .yajl_string      = _j4status_i3bar_output_click_events_string,
    .yajl_start_map   = _j4status_i3bar_output_click_events_start_map,
    .yajl_map_key     = _j4status_i3bar_output_click_events_map_key,
    .yajl_end_map     = _j4status_i3bar_output_click_events_end_map,
    .yajl_start_array = _j4status_i3bar_output_click_events_start_array,
    .yajl_end_array   = _j4status_i3bar_output_click_events_end_array,
};

static void
_j4status_i3bar_ouput_action(J4statusPluginContext *context, gchar *line)
{
    gsize length = strlen(line);
    yajl_status json_state;

    json_state = yajl_parse(context->json_handle, (const unsigned char *) line, length);

    if ( json_state == yajl_status_ok )
        return;
    g_free(context->parse_context.name);
    g_free(context->parse_context.instance);

    if ( json_state == yajl_status_error )
    {
        unsigned char *str_error;
        str_error = yajl_get_error(context->json_handle, 0, (const unsigned char *) line, length);
        g_warning("Couldn't parse section from i3bar: %s", str_error);
        yajl_free_error(context->json_handle, str_error);
    }
    else if ( json_state == yajl_status_client_canceled )
    {
        g_warning("i3bar JSON protocol error: %s", context->parse_context.error);
        g_free(context->parse_context.error);
    }
}

static void
_j4status_i3bar_output_stream_read_callback(GObject *obj, GAsyncResult *res, gpointer user_data)
{
    J4statusOutputPluginStream *stream = user_data;
    GDataInputStream *in = G_DATA_INPUT_STREAM(obj);
    GError *error = NULL;

    gchar *line;
    line = g_data_input_stream_read_line_finish(in, res, NULL, &error);
    if ( line == NULL )
    {
        if ( error == NULL )
            j4status_core_stream_free(stream->context->core, stream->stream);
        else
        {
            g_warning("Input error: %s", error->message);
            j4status_core_stream_reconnect(stream->context->core, stream->stream);
        }
        g_clear_error(&error);
        return;
    }

    _j4status_i3bar_ouput_action(stream->context, line);

    g_free(line);
    g_data_input_stream_read_line_async(in, G_PRIORITY_DEFAULT, NULL, _j4status_i3bar_output_stream_read_callback, stream);
}

static J4statusOutputPluginStream *
_j4status_i3bar_output_stream_new(J4statusPluginContext *context, J4statusCoreStream *core_stream)
{
    J4statusOutputPluginStream *stream;

    stream = g_slice_new0(J4statusOutputPluginStream);
    stream->context = context;
    stream->stream = core_stream;

    stream->out = g_data_output_stream_new(j4status_core_stream_get_output_stream(stream->context->core, stream->stream));
    stream->in = g_data_input_stream_new(j4status_core_stream_get_input_stream(stream->context->core, stream->stream));

    g_data_input_stream_read_line_async(stream->in, G_PRIORITY_DEFAULT, NULL, _j4status_i3bar_output_stream_read_callback, stream);

    return stream;
}

static void
_j4status_i3bar_output_stream_free(J4statusPluginContext *context, J4statusOutputPluginStream *stream)
{
    g_object_unref(stream->out);
    g_object_unref(stream->in);

    g_slice_free(J4statusOutputPluginStream, stream);
}

static void
_j4status_i3bar_output_update_colour(gchar **colour, GKeyFile *key_file, gchar *name)
{
    gchar *config;
    config = g_key_file_get_string(key_file, "i3bar", name, NULL);
    if ( config == NULL )
        return;

    g_free(*colour);

    *colour = g_strdup(j4status_colour_to_hex(j4status_colour_parse(config)));
    g_free(config);
}

static J4statusPluginContext *
_j4status_i3bar_output_init(J4statusCoreInterface *core)
{
    J4statusPluginContext *context;

    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->colours.no_state    = NULL;
    context->colours.unavailable = g_strdup("#0000FF");
    context->colours.bad         = g_strdup("#FF0000");
    context->colours.average     = g_strdup("#FFFF00");
    context->colours.good        = g_strdup("#00FF00");

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("i3bar");
    if ( key_file != NULL )
    {
        _j4status_i3bar_output_update_colour(&context->colours.no_state, key_file, "NoStateColour");
        _j4status_i3bar_output_update_colour(&context->colours.unavailable, key_file, "UnavailableColour");
        _j4status_i3bar_output_update_colour(&context->colours.bad, key_file, "BadColour");
        _j4status_i3bar_output_update_colour(&context->colours.average, key_file, "AverageColour");
        _j4status_i3bar_output_update_colour(&context->colours.good, key_file, "GoodColour");
        context->align = g_key_file_get_boolean(key_file, "i3bar", "Align", NULL);
        context->no_click_events = g_key_file_get_boolean(key_file, "i3bar", "NoClickEvents", NULL);
        g_key_file_free(key_file);
    }

    context->json_handle = yajl_alloc(&_j4status_i3bar_output_click_events_callbacks, NULL, context);

    return context;
}

static void
_j4status_i3bar_output_uninit(J4statusPluginContext *context)
{
    yajl_free(context->json_handle);

    g_free(context->line);
    g_free(context->header);

    g_free(context);
}

static gchar *
_j4status_i3bar_output_generate_header(J4statusPluginContext *context)
{
    yajl_gen json_gen;
    json_gen = yajl_gen_alloc(NULL);

    yajl_gen_map_open(json_gen);
    yajl_gen_string(json_gen, (const unsigned char *)"version", strlen("version"));
    yajl_gen_integer(json_gen, 1);
    yajl_gen_string(json_gen, (const unsigned char *)"stop_signal", strlen("stop_signal"));
    yajl_gen_integer(json_gen, SIGUSR2);
    yajl_gen_string(json_gen, (const unsigned char *)"cont_signal", strlen("cont_signal"));
    yajl_gen_integer(json_gen, SIGUSR1);
    if ( ! context->no_click_events )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"click_events", strlen("click_events"));
        yajl_gen_bool(json_gen, 1);
    }
    yajl_gen_map_close(json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(json_gen, &buffer, &length);

    gchar *header;

    header = g_strdup_printf("%s\n[[]\n", buffer);
    yajl_gen_free(json_gen);

    return header;
}

static gboolean
_j4status_i3bar_output_send_header(J4statusPluginContext *context, J4statusOutputPluginStream *stream, GError **error)
{
    if ( context->header == NULL )
        context->header = _j4status_i3bar_output_generate_header(context);
    return g_data_output_stream_put_string(stream->out, context->header, NULL, error);
}

static void
_j4status_i3bar_output_process_section(J4statusPluginContext *context, J4statusSection *section)
{
    const gchar *value;
    gchar *labelled_value = NULL;
    value = j4status_section_get_value(section);

    if ( value == NULL )
    {
        j4status_section_set_cache(section, NULL);
        return;
    }

    yajl_gen json_gen;

    json_gen = yajl_gen_alloc(NULL);

    const gchar *label;
    label = j4status_section_get_label(section);
    const gchar *label_colour;
    label_colour = j4status_colour_to_hex(j4status_section_get_label_colour(section));


    if ( ( label != NULL ) && ( label_colour != NULL ) )
    {
        gchar *label_with_sep;
        label_with_sep = g_strdup_printf("%s: ", label);

        /*
         * We create a fake section with just the label
         * so we open an array to have yajl generate a
         * comma between the two sections
         */
        yajl_gen_array_open(json_gen);
        yajl_gen_clear(json_gen); /* We need the comma alone */
        yajl_gen_map_open(json_gen);

        yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
        yajl_gen_string(json_gen, (const unsigned char *)label_colour, strlen("#000000"));

        yajl_gen_string(json_gen, (const unsigned char *)"full_text", strlen("full_text"));
        yajl_gen_string(json_gen, (const unsigned char *)label_with_sep, strlen(label_with_sep));

        yajl_gen_string(json_gen, (const unsigned char *)"separator", strlen("separator"));
        yajl_gen_bool(json_gen, FALSE);
        yajl_gen_string(json_gen, (const unsigned char *)"separator_block_width", strlen("separator_block_width"));
        yajl_gen_integer(json_gen, 0);

        yajl_gen_map_close(json_gen);

        g_free(label_with_sep);
    }
    else if ( label != NULL )
        value = labelled_value = g_strdup_printf("%s: %s", label, value);

    yajl_gen_map_open(json_gen);

    const gchar *name;
    name = j4status_section_get_name(section);
    if ( name != NULL )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"name", strlen("name"));
        yajl_gen_string(json_gen, (const unsigned char *)name, strlen(name));
    }

    const gchar *instance;
    instance = j4status_section_get_instance(section);
    if ( instance != NULL )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"instance", strlen("instance"));
        yajl_gen_string(json_gen, (const unsigned char *)instance, strlen(instance));
    }

    gint64 max_width;
    max_width = j4status_section_get_max_width(section);
    if ( context->align && ( max_width != 0 ) )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"min_width", strlen("min_width"));
        if ( max_width < 0 )
        {
            gsize l = - max_width + 1;
            if ( ( label != NULL ) && ( label_colour == NULL ) )
                l += strlen(label);
            gchar max_value[l];
            memset(max_value, 'm', l);
            max_value[l] ='\0';
            yajl_gen_string(json_gen, (const unsigned char *)max_value, l);
        }
        else
            yajl_gen_integer(json_gen, max_width);

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
        break;
        }
        if ( align != NULL )
        {
            yajl_gen_string(json_gen, (const unsigned char *)"align", strlen("align"));
            yajl_gen_string(json_gen, (const unsigned char *)align, strlen(align));
        }
    }

    J4statusState state = j4status_section_get_state(section);
    const gchar *colour = NULL;
    const gchar *background_colour = NULL;
    switch ( state & ~J4STATUS_STATE_FLAGS )
    {
    case J4STATUS_STATE_NO_STATE:
        colour = context->colours.no_state;
    break;
    case J4STATUS_STATE_UNAVAILABLE:
        colour = context->colours.unavailable;
    break;
    case J4STATUS_STATE_BAD:
        colour = context->colours.bad;
    break;
    case J4STATUS_STATE_AVERAGE:
        colour = context->colours.average;
    break;
    case J4STATUS_STATE_GOOD:
        colour = context->colours.good;
    break;
    case J4STATUS_STATE_URGENT:
    break;
    }
    if ( state & J4STATUS_STATE_URGENT )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"urgent", strlen("urgent"));
        yajl_gen_bool(json_gen, TRUE);
    }

    const gchar *forced_colour;
    forced_colour = j4status_colour_to_hex(j4status_section_get_colour(section));
    if ( forced_colour != NULL )
        colour = forced_colour;

    if ( colour != NULL )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
        yajl_gen_string(json_gen, (const unsigned char *)colour, strlen("#000000"));
    }

    forced_colour = j4status_colour_to_hex(j4status_section_get_background_colour(section));
    if ( forced_colour != NULL )
        background_colour = forced_colour;

    if ( background_colour != NULL )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"background", strlen("background"));
        yajl_gen_string(json_gen, (const unsigned char *)background_colour, strlen("#000000"));
    }

    const gchar *short_value;
    short_value = j4status_section_get_short_value(section);
    if ( short_value != NULL )
    {
        yajl_gen_string(json_gen, (const unsigned char *)"short_text", strlen("short_text"));
        yajl_gen_string(json_gen, (const unsigned char *)short_value, strlen(short_value));
    }

    yajl_gen_string(json_gen, (const unsigned char *)"full_text", strlen("full_text"));
    yajl_gen_string(json_gen, (const unsigned char *)value, strlen(value));
    g_free(labelled_value);

    yajl_gen_map_close(json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(json_gen, &buffer, &length);

    j4status_section_set_cache(section, g_strdup((const gchar *)buffer));

    yajl_gen_free(json_gen);
}

static void
_j4status_i3bar_output_generate_line(J4statusPluginContext *context, GList *sections)
{
    GString *line = g_string_sized_new(context->last_len);
    g_string_append_c(g_string_append_c(line, ','), '[');
    gboolean first = TRUE;
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {

        section = section_->data;
        if ( j4status_section_is_dirty(section) )
            _j4status_i3bar_output_process_section(context, section);

        const gchar *cache;
        cache = j4status_section_get_cache(section);
        if ( cache == NULL )
            continue;

        if ( first )
            first = FALSE;
        else
            g_string_append_c(line, ',');
        g_string_append(line, cache);
    }

    g_string_append_c(g_string_append_c(line, ']'), '\n');
    context->last_len = line->len;

    g_free(context->line);
    context->line = g_string_free(line, FALSE);
}

static gboolean
_j4status_i3bar_output_send_line(J4statusPluginContext *context, J4statusOutputPluginStream *stream, GError **error)
{
    return g_data_output_stream_put_string(stream->out, context->line, NULL, error);
}

J4STATUS_EXPORT void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_init_callback(interface, _j4status_i3bar_output_init);
    libj4status_output_plugin_interface_add_uninit_callback(interface, _j4status_i3bar_output_uninit);

    libj4status_output_plugin_interface_add_stream_new_callback(interface, _j4status_i3bar_output_stream_new);
    libj4status_output_plugin_interface_add_stream_free_callback(interface, _j4status_i3bar_output_stream_free);

    libj4status_output_plugin_interface_add_send_header_callback(interface, _j4status_i3bar_output_send_header);
    libj4status_output_plugin_interface_add_generate_line_callback(interface, _j4status_i3bar_output_generate_line);
    libj4status_output_plugin_interface_add_send_line_callback(interface, _j4status_i3bar_output_send_line);
}
