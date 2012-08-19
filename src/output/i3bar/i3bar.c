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
#include <string.h>
#include <yajl/yajl_gen.h>

#include <j4status-plugin.h>

static yajl_gen json_gen = NULL;

void
j4status_output_init(GList *section_)
{
    json_gen = yajl_gen_alloc(NULL);
    g_printf("{\"version\":1}\n[\n");
    yajl_gen_array_open(json_gen);
    yajl_gen_clear(json_gen);
}

void
j4status_output(GList *sections_)
{
    yajl_gen_array_open(json_gen);
    J4statusSection *section;
    for ( ; sections_ != NULL ; sections_ = g_list_next(sections_) )
    {
        GList **section__ = sections_->data;
        GList *section_ = *section__;
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

            yajl_gen_map_open(json_gen);

            if ( section->name != NULL )
            {
                yajl_gen_string(json_gen, (const unsigned char *)"name", strlen("name"));
                yajl_gen_string(json_gen, (const unsigned char *)section->name, strlen(section->name));
            }

            if ( section->instance != NULL )
            {
                yajl_gen_string(json_gen, (const unsigned char *)"instance", strlen("instance"));
                yajl_gen_string(json_gen, (const unsigned char *)section->instance, strlen(section->instance));
            }

            switch ( section->state )
            {
            case J4STATUS_STATE_UNAVAILABLE:
            break;
            case J4STATUS_STATE_UNKNOWN:
                yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(json_gen, (const unsigned char *)"#0000FF", 7);
            break;
            case J4STATUS_STATE_BAD:
                yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(json_gen, (const unsigned char *)"#FF0000", 7);
            break;
            case J4STATUS_STATE_AVERAGE:
                yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(json_gen, (const unsigned char *)"#FFFF00", 7);
            break;
            case J4STATUS_STATE_GOOD:
                yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(json_gen, (const unsigned char *)"#00FF00", 7);
            break;
            case J4STATUS_STATE_URGENT:
                yajl_gen_string(json_gen, (const unsigned char *)"color", strlen("color"));
                yajl_gen_string(json_gen, (const unsigned char *)"#FF0000", 7);
                yajl_gen_string(json_gen, (const unsigned char *)"urgent", strlen("urgent"));
                yajl_gen_bool(json_gen, TRUE);
            break;
            }

            yajl_gen_string(json_gen, (const unsigned char *)"full_text", strlen("full_text"));
            yajl_gen_string(json_gen, (const unsigned char *)section->line_cache, strlen(section->line_cache));

            yajl_gen_map_close(json_gen);
        }
    }
    yajl_gen_array_close(json_gen);

    const unsigned char *buffer;
    size_t length;
    yajl_gen_get_buf(json_gen, &buffer, &length);
    g_printf("%s\n", buffer);
    yajl_gen_clear(json_gen);
}
