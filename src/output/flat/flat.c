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

#include <j4status-plugin.h>

static void
_j4status_flat_print(J4statusPluginContext *context, GList *sections)
{
    GList *section_;
    J4statusSection *section;
    gboolean first = TRUE;
    for ( section_ = sections ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;
        const gchar *cache;
        if ( j4status_section_is_dirty(section) )
        {
            gchar *new_cache = NULL;
            const gchar *label;
            const gchar *value;

            value = j4status_section_get_value(section);
            if ( value != NULL )
            {
                label = j4status_section_get_label(section);
                if ( label != NULL )
                    new_cache = g_strdup_printf("%s: %s", label, value);
                else
                    new_cache = g_strdup(value);
            }
            j4status_section_set_cache(section, new_cache);
            cache = new_cache;
        }
        else
            cache = j4status_section_get_cache(section);
        if ( cache == NULL )
            continue;
        if ( first )
            first = FALSE;
        else
            g_printf(" | ");
        g_printf("%s", cache);
    }
    g_printf("\n");
}

void
j4status_output_plugin(J4statusOutputPluginInterface *interface)
{
    libj4status_output_plugin_interface_add_print_callback(interface, _j4status_flat_print);
}
