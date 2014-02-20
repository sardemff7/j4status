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

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-output.h>

static void
_j4status_debug_print(J4statusPluginContext *context, GList *sections)
{
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;

        const gchar *state = NULL;
        switch ( j4status_section_get_state(section) )
        {
        case J4STATUS_STATE_NO_STATE:
            state = "no state";
        break;
        case J4STATUS_STATE_UNAVAILABLE:
            state = "unavailable";
        break;
        case J4STATUS_STATE_BAD:
            state = "bad";
        break;
        case J4STATUS_STATE_AVERAGE:
            state = "average";
        break;
        case J4STATUS_STATE_GOOD:
            state = "good";
        break;
        case J4STATUS_STATE_URGENT:
            state = "urgent";
        break;
        }
        g_printf("--"
            "\nName: %s"
            "\nInstance: %s"
            "\nState: %s"
            "\nLabel: %s"
            "\nValue: %s"
            "\n--\n",
            j4status_section_get_name(section),
            j4status_section_get_instance(section),
            state,
            j4status_section_get_label(section),
            j4status_section_get_value(section));
    }
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_print_callback(interface, _j4status_debug_print);
}
