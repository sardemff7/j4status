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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-output.h>

#define BOOL_TO_S(bool) ((bool) ? "yes" : "no")

static void
_j4status_debug_print(J4statusPluginContext *context, GList *sections)
{
    GString *line = g_string_new("");
    gboolean first = TRUE;
    GList *section_;
    J4statusSection *section;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;

        if ( ! j4status_section_is_dirty(section) )
            goto print;

        const gchar *align = NULL;
        switch ( j4status_section_get_align(section) )
        {
        case J4STATUS_ALIGN_LEFT:
            align = "left";
        break;
        case J4STATUS_ALIGN_RIGHT:
            align = "right";
        break;
        case J4STATUS_ALIGN_CENTER:
            align = "center";
        break;
        }

        J4statusState state = j4status_section_get_state(section);
        const gchar *state_s = NULL;
        switch ( state & ~J4STATUS_STATE_FLAGS )
        {
        case J4STATUS_STATE_NO_STATE:
            state_s = "no state";
        break;
        case J4STATUS_STATE_UNAVAILABLE:
            state_s = "unavailable";
        break;
        case J4STATUS_STATE_BAD:
            state_s = "bad";
        break;
        case J4STATUS_STATE_AVERAGE:
            state_s = "average";
        break;
        case J4STATUS_STATE_GOOD:
            state_s = "good";
        break;
        }

        gchar *cache;
        cache = g_strdup_printf("--"
            "\nName: %s"
            "\nInstance: %s"
            "\nLabel: %s"
            "\nLabel colour: %s"
            "\nAlignment: %s"
            "\nMaximum width: %jd"
            "\n"
            "\nState: %s"
            "\nUrgent: %s"
            "\nColour: %s"
            "\nValue: %s"
            "\n--",
            j4status_section_get_name(section),
            j4status_section_get_instance(section),
            j4status_section_get_label(section),
            j4status_colour_to_hex(j4status_section_get_label_colour(section)),
            align,
            j4status_section_get_max_width(section),
            state_s,
            BOOL_TO_S(state & J4STATUS_STATE_URGENT),
            j4status_colour_to_hex(j4status_section_get_colour(section)),
            j4status_section_get_value(section));

        j4status_section_set_cache(section, cache);

    print:
        g_string_append(line, j4status_section_get_cache(section));
        if ( first )
            first = FALSE;
        else
            g_string_append_c(line, '\n');
    }

    g_printf("%s", line->str);
    g_string_free(line, TRUE);
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_print_callback(interface, _j4status_debug_print);
}
