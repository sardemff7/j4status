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

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin.h>

void
j4status_output(GList *section_)
{
    J4statusSection *section;
    for ( ; section_ != NULL ; section_ = g_list_next(section_) )
    {
        section = section_->data;
        if ( section->dirty )
        {
            g_free(section->line_cache);
            section->dirty = FALSE;
            if ( section->value == NULL )
            {
                section->line_cache = NULL;
                continue;
            }
            if ( section->label != NULL )
                section->line_cache = g_strdup_printf("%s: %s", section->label, section->value);
            else
                section->line_cache = g_strdup(section->value);
        }
        if ( section->line_cache == NULL )
            continue;
        g_printf("%s", section->line_cache);
        if ( g_list_next(section_) != NULL )
            g_printf(" | ");
    }
    g_printf("\n");
}
