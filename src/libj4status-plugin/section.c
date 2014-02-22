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

/*
 * Input plugins API
 */

J4statusSection *
j4status_section_new(J4statusCoreInterface *core)
{
    g_return_val_if_fail(core != NULL, NULL);

    J4statusSection *self;

    self = g_new0(J4statusSection, 1);
    self->core = core;
    self->state = J4STATUS_STATE_NO_STATE;

    return self;
}

void
j4status_section_free(J4statusSection *self)
{
    g_return_if_fail(self != NULL);

    if ( self->freeze )
        self->core->remove_section(self->core->context, self);

    g_free(self->cache);

    g_free(self->value);

    g_free(self->label);
    g_free(self->instance);

    g_free(self);
}

/* API before inserting the section in the list */
void
j4status_section_set_name(J4statusSection *self, const gchar *name)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);
    g_return_if_fail(name != NULL);

    self->name = name;
}

void
j4status_section_set_instance(J4statusSection *self, const gchar *instance)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    g_free(self->instance);
    self->instance = g_strdup(instance);
}

void
j4status_section_set_label(J4statusSection *self, const gchar *label)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    g_free(self->label);
    self->label = g_strdup(label);
}

void
j4status_section_insert(J4statusSection *self)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);
    g_return_if_fail(self->name != NULL);

    self->freeze = TRUE;
    self->core->add_section(self->core->context, self);
}

/* API once the section is inserted in the list */
void
j4status_section_set_state(J4statusSection *self, J4statusState state)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    self->dirty = TRUE;
    self->state = state;
}

void
j4status_section_set_value(J4statusSection *self, gchar *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    self->dirty = TRUE;
    g_free(self->value);
    self->value = value;
}


/*
 * Output plugins API
 */

const gchar *
j4status_section_get_name(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->name;
}

const gchar *
j4status_section_get_instance(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->instance;
}

J4statusState
j4status_section_get_state(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, J4STATUS_STATE_NO_STATE);
    g_return_val_if_fail(self->freeze, J4STATUS_STATE_NO_STATE);

    return self->state;
}

const gchar *
j4status_section_get_label(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->label;
}

const gchar *
j4status_section_get_value(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->value;
}

gboolean
j4status_section_is_dirty(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, TRUE);
    g_return_val_if_fail(self->freeze, NULL);

    return self->dirty;
}

void
j4status_section_set_cache(J4statusSection *self, gchar *cache)
{
    g_return_if_fail(self != NULL);
    g_return_val_if_fail(self->freeze, NULL);

    g_free(self->cache);
    self->cache = cache;
    self->dirty = FALSE;
}

const gchar *
j4status_section_get_cache(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->cache;
}
