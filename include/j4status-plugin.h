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

void libj4status_core_trigger_display(J4statusCoreInterface *core);


typedef struct _J4statusPluginContext J4statusPluginContext;

typedef J4statusPluginContext *(*J4statusPluginInitFunc)(J4statusCoreInterface *core);
typedef void(*J4statusPluginPrintFunc)(J4statusPluginContext *context, GList *sections);
typedef void(*J4statusPluginSimpleFunc)(J4statusPluginContext *context);
typedef GList **(*J4statusPluginGetSectionsFunc)(J4statusPluginContext *context);

typedef struct _J4statusOutputPluginInterface J4statusOutputPluginInterface;

typedef struct _J4statusInputPluginInterface J4statusInputPluginInterface;

#define LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(type, Type, action, Action) void libj4status_##type##_plugin_interface_add_##action##_callback(J4status##Type##PluginInterface *interface, J4statusPlugin##Action##Func callback)

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, init, Init);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, uninit, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(output, Output, print, Print);

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, init, Init);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, uninit, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, get_sections, GetSections);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, start, Simple);
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(input, Input, stop, Simple);

typedef enum {
    J4STATUS_STATE_NO_STATE = -1,
    J4STATUS_STATE_UNAVAILABLE = 0,
    J4STATUS_STATE_BAD,
    J4STATUS_STATE_AVERAGE,
    J4STATUS_STATE_GOOD,
    J4STATUS_STATE_URGENT
} J4statusState;

typedef struct _J4statusSection J4statusSection;

J4statusSection *j4status_section_new(J4statusCoreInterface *core, const gchar *name, const gchar *instance, gpointer user_data);
void j4status_section_free(J4statusSection *self);
const gchar *j4status_section_get_name(const J4statusSection *self);
gpointer j4status_section_get_user_data(const J4statusSection *self);
const gchar *j4status_section_get_instance(const J4statusSection *self);
J4statusState j4status_section_get_state(const J4statusSection *self);
const gchar *j4status_section_get_label(const J4statusSection *self);
const gchar *j4status_section_get_value(const J4statusSection *self);
const gchar *j4status_section_get_cache(const J4statusSection *self);
gboolean j4status_section_is_dirty(const J4statusSection *self);
void j4status_section_set_state(J4statusSection *self, J4statusState state);
void j4status_section_set_label(J4statusSection *self, const gchar *label);
void j4status_section_set_value(J4statusSection *self, gchar *value);
void j4status_section_set_cache(J4statusSection *self, gchar *cache);

#endif /* __J4STATUS_J4STATUS_PLUGIN_H__ */
