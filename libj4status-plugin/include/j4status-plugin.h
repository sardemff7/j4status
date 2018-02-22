/*
 * libj4status-plugin - Library to implement a j4status plugin
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

#ifndef __J4STATUS_J4STATUS_PLUGIN_H__
#define __J4STATUS_J4STATUS_PLUGIN_H__

#include <glib.h>

typedef struct _J4statusCoreInterface J4statusCoreInterface;


typedef struct _J4statusPluginContext J4statusPluginContext;

typedef J4statusPluginContext *(*J4statusPluginInitFunc)(J4statusCoreInterface *core);
typedef void(*J4statusPluginSimpleFunc)(J4statusPluginContext *context);


#define LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(type, Type, action, Action) void libj4status_##type##_plugin_interface_add_##action##_callback(J4status##Type##PluginInterface *interface, J4statusPlugin##Action##Func callback)


typedef enum {
    J4STATUS_ALIGN_CENTER = 0,
    J4STATUS_ALIGN_LEFT,
    J4STATUS_ALIGN_RIGHT
} J4statusAlign;

typedef enum {
    J4STATUS_STATE_NO_STATE = 0,
    J4STATUS_STATE_UNAVAILABLE,
    J4STATUS_STATE_BAD,
    J4STATUS_STATE_AVERAGE,
    J4STATUS_STATE_GOOD,
#define _J4STATUS_STATE_SIZE (J4STATUS_STATE_GOOD+1)
    J4STATUS_STATE_URGENT = (1 << 31)
} J4statusState;

#define J4STATUS_STATE_FLAGS (J4STATUS_STATE_URGENT)

GKeyFile *j4status_config_get_key_file(const gchar *section);
gboolean j4status_config_key_file_get_enum(GKeyFile *key_file, const gchar *group_name, const gchar *key, const gchar * const *values, guint64 size, guint64 *value);
GHashTable *j4status_config_key_file_get_actions(GKeyFile *key_file, const gchar *group_name, const gchar * const *values, guint64 size);

typedef struct _NkTokenList J4statusFormatString;

typedef GVariant *(*J4statusFormatStringReplaceCallback)(const gchar *token, guint64 value, gconstpointer user_data);

J4statusFormatString *j4status_format_string_parse(gchar *string, const gchar * const *tokens, guint64 size, const gchar *default_string, guint64 *used_tokens);
J4statusFormatString *j4status_format_string_ref(J4statusFormatString *format_string);
void j4status_format_string_unref(J4statusFormatString *format_string);
gchar *j4status_format_string_replace(const J4statusFormatString *format_string, J4statusFormatStringReplaceCallback callback, gconstpointer user_data);

typedef struct {
    gboolean set;
    guint8 red;
    guint8 green;
    guint8 blue;
    guint8 alpha;
} J4statusColour;

void j4status_colour_reset(J4statusColour *colour);
J4statusColour j4status_colour_parse(const gchar *string);
J4statusColour j4status_colour_parse_length(const gchar *string, gint size);
const gchar *j4status_colour_to_hex(J4statusColour colour);
const gchar *j4status_colour_to_rgb(J4statusColour colour);

typedef struct _J4statusSection J4statusSection;

#endif /* __J4STATUS_J4STATUS_PLUGIN_H__ */
