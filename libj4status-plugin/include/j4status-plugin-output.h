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

#ifndef __J4STATUS_J4STATUS_PLUGIN_OUTPUT_H__
#define __J4STATUS_J4STATUS_PLUGIN_OUTPUT_H__

#include <j4status-plugin.h>

void j4status_core_trigger_action(J4statusCoreInterface *core, const gchar *section_id, const gchar *action_id);

typedef void(*J4statusPluginPrintFunc)(J4statusPluginContext *context, GList *sections);

typedef struct _J4statusOutputPluginInterface J4statusOutputPluginInterface;

#define LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(type, Type, action, Action) void libj4status_##type##_plugin_interface_add_##action##_callback(J4status##Type##PluginInterface *interface, J4statusPlugin##Action##Func callback)

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, init, Init);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, uninit, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, print, Print);

const gchar *j4status_section_get_name(const J4statusSection *section);
const gchar *j4status_section_get_instance(const J4statusSection *section);
const gchar *j4status_section_get_label(const J4statusSection *section);
J4statusColour j4status_section_get_label_colour(const J4statusSection *section);
J4statusAlign j4status_section_get_align(const J4statusSection *section);
gint64 j4status_section_get_max_width(const J4statusSection *section);
J4statusState j4status_section_get_state(const J4statusSection *section);
J4statusColour j4status_section_get_colour(const J4statusSection *section);
const gchar *j4status_section_get_value(const J4statusSection *section);

gboolean j4status_section_is_dirty(const J4statusSection *section);
void j4status_section_set_cache(J4statusSection *section, gchar *cache);
const gchar *j4status_section_get_cache(const J4statusSection *section);

#endif /* __J4STATUS_J4STATUS_PLUGIN_OUTPUT_H__ */
