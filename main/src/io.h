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

#ifndef __J4STATUS_IO_H__
#define __J4STATUS_IO_H__

#include "types.h"

J4statusIOContext *j4status_io_new(J4statusCoreContext *core, const gchar * const *servers_desc, const gchar * const *streams_desc);
void j4status_io_free(J4statusIOContext *io);

void j4status_io_update_line(J4statusIOContext *io, gchar *line);

#endif /* __J4STATUS_IO_H__ */
