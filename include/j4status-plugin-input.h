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

J4statusSection *j4status_section_new(J4statusCoreInterface *core, const gchar *name, const gchar *instance);
void j4status_section_free(J4statusSection *self);
void j4status_section_set_state(J4statusSection *self, J4statusState state);
void j4status_section_set_label(J4statusSection *self, const gchar *label);
void j4status_section_set_value(J4statusSection *self, gchar *value);

#endif /* __J4STATUS_J4STATUS_PLUGIN_INPUT_H__ */
