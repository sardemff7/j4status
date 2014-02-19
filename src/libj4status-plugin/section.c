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

#include <j4status-plugin.h>
#include <j4status-plugin-private.h>

J4statusSection *
j4status_section_new(const gchar *name, const gchar *instance, gpointer user_data)
{
    J4statusSection *self;

    self = g_new0(J4statusSection, 1);
    self->name = name;
    self->instance = g_strdup(instance);
    self->user_data = user_data;

    return self;
}

void
j4status_section_free(J4statusSection *self)
{
    g_free(self->cache);

    g_free(self->value);
    g_free(self->label);

    g_free(self->instance);

    g_free(self);
}

const gchar *
j4status_section_get_name(const J4statusSection *self)
{
    return self->name;
}

gpointer
j4status_section_get_user_data(const J4statusSection *self)
{
    return self->user_data;
}

const gchar *
j4status_section_get_instance(const J4statusSection *self)
{
    return self->instance;
}

J4statusState
j4status_section_get_state(const J4statusSection *self)
{
    return self->state;
}

const gchar *
j4status_section_get_label(const J4statusSection *self)
{
    return self->label;
}

const gchar *
j4status_section_get_value(const J4statusSection *self)
{
    return self->value;
}

const gchar *
j4status_section_get_cache(const J4statusSection *self)
{
    return self->cache;
}

gboolean
j4status_section_is_dirty(const J4statusSection *self)
{
    return self->dirty;
}

void
j4status_section_set_state(J4statusSection *self, J4statusState state)
{
    self->dirty = TRUE;
    self->state = state;
}

void
j4status_section_set_label(J4statusSection *self, const gchar *label)
{
    self->dirty = TRUE;
    g_free(self->label);
    self->label = g_strdup(label);
}

void
j4status_section_set_value(J4statusSection *self, gchar *value)
{
    self->dirty = TRUE;
    g_free(self->value);
    self->value = value;
}

void
j4status_section_set_cache(J4statusSection *self, gchar *cache)
{
    g_free(self->cache);
    self->cache = cache;
    self->dirty = FALSE;
}
