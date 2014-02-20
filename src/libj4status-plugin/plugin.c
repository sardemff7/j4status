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

#include <glib.h>

#include <j4status-plugin-output.h>
#include <j4status-plugin-input.h>
#include <j4status-plugin-private.h>

#define LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(type, Type, action, Action) LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK(type, Type, action, Action) { interface->action = callback; }

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(output, Output, init, Init)
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(output, Output, uninit, Simple)
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(output, Output, print, Print)

LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(input, Input, init, Init)
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(input, Input, uninit, Simple)
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(input, Input, start, Simple)
LIBJ4STATUS_PLUGIN_INTERFACE_ADD_CALLBACK_DEF(input, Input, stop, Simple)
