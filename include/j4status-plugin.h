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
    J4STATUS_STATE_URGENT = (1 << 31)
} J4statusState;

#define J4STATUS_STATE_FLAGS (J4STATUS_STATE_URGENT)

typedef struct {
    gboolean set;
    guint8 red;
    guint8 green;
    guint8 blue;
} J4statusColour;

void j4status_colour_reset(J4statusColour *colour);
J4statusColour j4status_colour_parse(const gchar *colour);
J4statusColour j4status_colour_parse_length(const gchar *colour, gint size);
const gchar *j4status_colour_to_hex(J4statusColour colour);
const gchar *j4status_colour_to_rgb(J4statusColour colour);

typedef struct _J4statusSection J4statusSection;

#endif /* __J4STATUS_J4STATUS_PLUGIN_H__ */
