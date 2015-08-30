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

#ifndef __J4STATUS_J4STATUS_PLUGIN_PRIVATE_H__
#define __J4STATUS_J4STATUS_PLUGIN_PRIVATE_H__

struct _J4statusSection {
    J4statusCoreInterface *core;
    gboolean freeze;
    gchar *id;
    /* Reserved for the core */
    gint64 weight;
    GList *link;

    /* Input plugins can only touch these
     * before inserting the section in the list */
    gchar *name;
    gchar *instance;
    gchar *label;
    J4statusColour label_colour;
    J4statusAlign align;
    gint64 max_width;
    struct {
        J4statusSectionActionCallback callback;
        gpointer user_data;
    } action;

    /* Input plugins can only touch these
     * once the section is inserted in the list */
    J4statusState state;
    J4statusColour colour;
    gchar *value;
    gchar *short_value;

    /* Reserved for the output plugin */
    gboolean dirty;
    gchar *cache;
};

typedef struct _J4statusCoreContext J4statusCoreContext;

typedef void (*J4statusCoreFunc)(J4statusCoreContext *context);
typedef gboolean (*J4statusCoreSectionAddFunc)(J4statusCoreContext *context, J4statusSection *section);
typedef void (*J4statusCoreSectionFunc)(J4statusCoreContext *context, J4statusSection *section);
typedef void (*J4statusCoreTriggerDisplayFunc)(J4statusCoreContext *context, gboolean force);
typedef void (*J4statusCoreTriggerActionFunc)(J4statusCoreContext *context, const gchar *section_id, const gchar *event_id);

struct _J4statusCoreInterface {
    J4statusCoreContext *context;
    J4statusCoreSectionAddFunc add_section;
    J4statusCoreSectionFunc remove_section;
    J4statusCoreTriggerDisplayFunc trigger_display;
    J4statusCoreTriggerActionFunc trigger_action;
};


struct _J4statusOutputPluginInterface {
    J4statusPluginInitFunc     init;
    J4statusPluginSimpleFunc   uninit;

    J4statusPluginGenerateFunc generate;
};

struct _J4statusInputPluginInterface {
    J4statusPluginInitFunc   init;
    J4statusPluginSimpleFunc uninit;

    J4statusPluginSimpleFunc start;
    J4statusPluginSimpleFunc stop;
};

#endif /* __J4STATUS_J4STATUS_PLUGIN_PRIVATE_H__ */
