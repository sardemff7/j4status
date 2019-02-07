/*
 * libj4status-plugin - Library to implement a j4status plugin
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

#include "config.h"

#include <glib.h>

#include "j4status-plugin-output.h"
#include "j4status-plugin-input.h"
#include "j4status-plugin-private.h"

J4STATUS_EXPORT void
j4status_core_trigger_action(J4statusCoreInterface *core, const gchar *section_id, const gchar *event_id)
{
    return core->trigger_action(core->context, section_id, event_id);
}


J4STATUS_EXPORT GInputStream *
j4status_core_stream_get_input_stream(J4statusCoreInterface *core, J4statusCoreStream *stream)
{
    return core->stream_get_input_stream(stream);
}

J4STATUS_EXPORT GOutputStream *
j4status_core_stream_get_output_stream(J4statusCoreInterface *core, J4statusCoreStream *stream)
{
    return core->stream_get_output_stream(stream);
}

J4STATUS_EXPORT void
j4status_core_stream_reconnect(J4statusCoreInterface *core, J4statusCoreStream *stream)
{
    return core->stream_reconnect(stream);
}

J4STATUS_EXPORT void
j4status_core_stream_free(J4statusCoreInterface *core, J4statusCoreStream *stream)
{
    return core->stream_free(stream);
}
