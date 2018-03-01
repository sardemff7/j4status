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

#ifndef __J4STATUS_PLUGINS_H__
#define __J4STATUS_PLUGINS_H__

typedef struct {
    gpointer module;
    J4statusPluginContext *context;
    J4statusOutputPluginInterface interface;
} J4statusOutputPlugin;

typedef struct {
    gpointer module;
    J4statusPluginContext *context;
    J4statusInputPluginInterface interface;
} J4statusInputPlugin;

J4statusOutputPlugin *j4status_plugins_get_output_plugin(J4statusCoreInterface *core, const gchar *name);

GList *j4status_plugins_get_input_plugins(J4statusCoreInterface *core, gchar **names);

#endif /* __J4STATUS_PLUGINS_H__ */
