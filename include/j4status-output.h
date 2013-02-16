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

#ifndef __EVENTD_EVENTD_PLUGIN_H__
#define __EVENTD_EVENTD_PLUGIN_H__

#include <libj4status-event-types.h>

typedef struct _EventdCoreContext EventdCoreContext;
typedef struct _EventdCoreInterface EventdCoreInterface;

typedef struct _EventdPluginContext EventdPluginContext;

typedef EventdPluginContext *(*EventdPluginInitFunc)(EventdCoreContext *core, EventdCoreInterface *interface);
typedef void (*EventdPluginFunc)(EventdPluginContext *context);
typedef GOptionGroup *(*EventdPluginGetOptionGroupFunc)(EventdPluginContext *context);
typedef void (*EventdPluginControlCommandFunc)(EventdPluginContext *context, const gchar *command);
typedef void (*EventdPluginGlobalParseFunc)(EventdPluginContext *context, GKeyFile *);
typedef void (*EventdPluginEventParseFunc)(EventdPluginContext *context, const gchar *id, GKeyFile *);
typedef void (*EventdPluginEventDispatchFunc)(EventdPluginContext *context, EventdEvent *event);

typedef struct {
    EventdPluginInitFunc init;
    EventdPluginFunc uninit;

    EventdPluginGetOptionGroupFunc get_option_group;

    EventdPluginFunc start;
    EventdPluginFunc stop;

    EventdPluginControlCommandFunc control_command;

    EventdPluginFunc config_reset;

    EventdPluginGlobalParseFunc global_parse;
    EventdPluginEventParseFunc event_parse;

    EventdPluginEventDispatchFunc event_action;

    /* Private stuff */
    void *module;
    EventdPluginContext *context;
} EventdPlugin;

typedef void (*EventdPluginGetInfoFunc)(EventdPlugin *plugin);

#endif /* __EVENTD_EVENTD_PLUGIN_H__ */
