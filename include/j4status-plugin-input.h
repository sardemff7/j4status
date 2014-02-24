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

#ifndef __J4STATUS_J4STATUS_PLUGIN_INPUT_H__
#define __J4STATUS_J4STATUS_PLUGIN_INPUT_H__

#include <j4status-plugin.h>

void libj4status_core_trigger_display(J4statusCoreInterface *core);


typedef struct _J4statusInputPluginInterface J4statusInputPluginInterface;

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, init, Init);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, uninit, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, start, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, stop, Simple);

J4statusSection *j4status_section_new(J4statusCoreInterface *core);
void j4status_section_free(J4statusSection *section);

/* API before inserting the section in the list */
void j4status_section_set_name(J4statusSection *section, const gchar *name);
void j4status_section_set_instance(J4statusSection *section, const gchar *instance);
void j4status_section_set_label(J4statusSection *section, const gchar *label);
void j4status_section_set_label_colour(J4statusSection *section, J4statusColour colour);
void j4status_section_set_align(J4statusSection *section, J4statusAlign align);
void j4status_section_set_max_width(J4statusSection *section, gint64 max_width);
void j4status_section_insert(J4statusSection *section);

/* API once the section is inserted in the list */
void j4status_section_set_state(J4statusSection *section, J4statusState state);
void j4status_section_set_colour(J4statusSection *section, J4statusColour colour);
void j4status_section_set_value(J4statusSection *section, gchar *value);

#endif /* __J4STATUS_J4STATUS_PLUGIN_INPUT_H__ */
