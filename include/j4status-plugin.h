/*
 * j4status - Status line generator
 *
 * Copyright © 2012 Quentin "Sardem FF7" Glidic
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

typedef enum {
    J4STATUS_STATE_NO_STATE = -1,
    J4STATUS_STATE_UNAVAILABLE = 0,
    J4STATUS_STATE_BAD,
    J4STATUS_STATE_AVERAGE,
    J4STATUS_STATE_GOOD,
    J4STATUS_STATE_URGENT
} J4statusState;

typedef struct {
    const gchar *name;
    gchar *instance;
    gchar *label;
    gchar *value;
    J4statusState state;

    gboolean dirty;
    /*
     * cache for the output plugin
     * refreshed when dirty is set to TRUE
     */
    gchar *line_cache;
} J4statusSection;

typedef struct _J4statusPluginContext J4statusPluginContext;

typedef J4statusPluginContext *(*J4statusPluginInitFunc)();
typedef void(*J4statusPluginPrintFunc)(J4statusPluginContext *context, GList *sections);
typedef void(*J4statusPluginFunc)(J4statusPluginContext *context);
typedef GList **(*J4statusPluginGetSectionsFunc)(J4statusPluginContext *context);

typedef struct {
    J4statusPluginInitFunc init;
    J4statusPluginFunc     uninit;

    J4statusPluginPrintFunc print;

    /* Private stuff */
    gpointer module;
    J4statusPluginContext *context;
} J4statusOutputPlugin;

typedef struct {
    J4statusPluginInitFunc init;
    J4statusPluginFunc     uninit;

    J4statusPluginGetSectionsFunc get_sections;

    J4statusPluginFunc start;
    J4statusPluginFunc stop;

    /* Private stuff */
    gpointer module;
    J4statusPluginContext *context;
} J4statusInputPlugin;

#endif /* __J4STATUS_J4STATUS_PLUGIN_H__ */
