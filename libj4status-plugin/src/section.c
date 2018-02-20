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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <glib.h>
#include <glib/gprintf.h>

#include <j4status-plugin-output.h>
#include <j4status-plugin-input.h>
#include <j4status-plugin-private.h>
#include <j4status-plugin.h>

static gboolean
_j4status_section_get_override(J4statusSection *self)
{
    gchar group[strlen("Override ") + strlen(self->id) + 1];
    g_sprintf(group, "Override %s", self->id);

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file(group);
    if ( key_file == NULL )
        return TRUE;

    gboolean insert = FALSE;

    GError *error = NULL;

    gboolean disable;
    disable = g_key_file_get_boolean(key_file, group, "Disable", &error);
    if ( ( error == NULL ) && disable )
        goto end;
    g_clear_error(&error);

    insert = TRUE;

    gchar *label;
    label = g_key_file_get_string(key_file, group, "Label", NULL);
    if ( label != NULL )
    {
        g_free(self->label);
        if ( g_strcmp0(label, "") == 0 )
            label = (g_free(label), NULL);
        self->label = label;
    }

    gchar *label_colour;
    label_colour = g_key_file_get_string(key_file, group, "LabelColour", NULL);
    if ( label_colour != NULL )
    {
        self->label_colour = j4status_colour_parse(label_colour);
        g_free(label_colour);
    }

    gchar *align;
    align = g_key_file_get_string(key_file, group, "Alignment", NULL);
    if ( align != NULL )
    {
        if ( g_ascii_strcasecmp(align, "left") == 0 )
            self->align = J4STATUS_ALIGN_LEFT;
        else if ( g_ascii_strcasecmp(align, "right") == 0 )
            self->align = J4STATUS_ALIGN_RIGHT;
        else if ( g_ascii_strcasecmp(align, "center") == 0 )
            self->align = J4STATUS_ALIGN_CENTER;
        g_free(label_colour);
    }

    gint64 max_width;
    max_width = g_key_file_get_int64(key_file, group, "MaxWidth", &error);
    if ( error == NULL )
        self->max_width = max_width;
    g_clear_error(&error);

end:
    g_key_file_free(key_file);
    return insert;
}

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

    return self;
}

void
j4status_section_free(J4statusSection *self)
{
    g_return_if_fail(self != NULL);

    if ( ( self->output.user_data != NULL ) && ( self->output.notify != NULL ) )
        self->output.notify(self->output.user_data);

    if ( self->freeze )
        self->core->remove_section(self->core->context, self);

    g_free(self->cache);

    g_free(self->short_value);
    g_free(self->value);

    g_free(self->label);
    g_free(self->instance);
    g_free(self->name);

    g_free(self->id);

    g_free(self);
}

/* API before inserting the section in the list */
void
j4status_section_set_name(J4statusSection *self, const gchar *name)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);
    g_return_if_fail(name != NULL);

    self->name = g_strdup(name);
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
j4status_section_set_label_colour(J4statusSection *self, J4statusColour colour)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    self->label_colour = colour;
}

void
j4status_section_set_align(J4statusSection *self, J4statusAlign align)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    self->align = align;
}

void
j4status_section_set_max_width(J4statusSection *self, gint64 max_width)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    self->max_width = max_width;
}

void
j4status_section_set_action_callback(J4statusSection *self, J4statusSectionActionCallback callback, gpointer user_data)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(! self->freeze);

    self->action.callback = callback;
    self->action.user_data = user_data;
}

gboolean
j4status_section_insert(J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(! self->freeze, FALSE);
    g_return_val_if_fail(self->name != NULL, FALSE);

    if ( self->instance != NULL )
        self->id = g_strdup_printf("%s:%s", self->name, self->instance);
    else
        self->id = g_strdup(self->name);

    if ( ! _j4status_section_get_override(self) )
        return FALSE;

    self->freeze = self->core->add_section(self->core->context, self);
    return self->freeze;
}

/* API once the section is inserted in the list */
void
j4status_section_set_state(J4statusSection *self, J4statusState state)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    if ( state & J4STATUS_STATE_URGENT )
        self->core->trigger_generate(self->core->context, TRUE);
    else if ( ! self->dirty )
        self->core->trigger_generate(self->core->context, FALSE);

    self->dirty = TRUE;

    self->state = state;
}

void
j4status_section_set_colour(J4statusSection *self, J4statusColour colour)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    if ( ! self->dirty )
        self->core->trigger_generate(self->core->context, FALSE);

    self->dirty = TRUE;

    self->colour = colour;
}

void
j4status_section_set_background_colour(J4statusSection *self, J4statusColour colour)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    if ( ! self->dirty )
        self->core->trigger_generate(self->core->context, FALSE);

    self->dirty = TRUE;

    self->background_colour = colour;
}

void
j4status_section_set_value(J4statusSection *self, gchar *value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    if ( ! self->dirty )
        self->core->trigger_generate(self->core->context, FALSE);

    self->dirty = TRUE;

    if ( ( value != NULL ) && ( *value == '\0' ) )
        value = (g_free(value), NULL);

    g_free(self->value);
    self->value = value;
}

void
j4status_section_set_short_value(J4statusSection *self, gchar *short_value)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    if ( ! self->dirty )
        self->core->trigger_generate(self->core->context, FALSE);

    self->dirty = TRUE;

    g_free(self->short_value);
    self->short_value = short_value;
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

const gchar *
j4status_section_get_label(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->label;
}

J4statusColour
j4status_section_get_label_colour(const J4statusSection *self)
{
    J4statusColour def = { FALSE, 0, 0, 0, 0 };
    g_return_val_if_fail(self != NULL, def);
    g_return_val_if_fail(self->freeze, def);

    return self->label_colour;
}

J4statusAlign
j4status_section_get_align(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, J4STATUS_ALIGN_CENTER);

    return self->align;
}

gint64
j4status_section_get_max_width(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, 0);
    g_return_val_if_fail(self->freeze, 0);

    return self->max_width;
}

J4statusState
j4status_section_get_state(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, J4STATUS_STATE_NO_STATE);
    g_return_val_if_fail(self->freeze, J4STATUS_STATE_NO_STATE);

    return self->state;
}

J4statusColour
j4status_section_get_colour(const J4statusSection *self)
{
    J4statusColour def = { FALSE, 0, 0, 0, 0 };
    g_return_val_if_fail(self != NULL, def);
    g_return_val_if_fail(self->freeze, def);

    return self->colour;
}

J4statusColour
j4status_section_get_background_colour(const J4statusSection *self)
{
    J4statusColour def = { FALSE, 0, 0, 0, 0 };
    g_return_val_if_fail(self != NULL, def);
    g_return_val_if_fail(self->freeze, def);

    return self->background_colour;
}

const gchar *
j4status_section_get_value(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->value;
}

const gchar *
j4status_section_get_short_value(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->short_value;
}

gboolean
j4status_section_is_dirty(const J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, TRUE);
    g_return_val_if_fail(self->freeze, TRUE);

    return self->dirty;
}

void
j4status_section_set_cache(J4statusSection *self, gchar *cache)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

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

void
j4status_section_set_output_user_data(J4statusSection *self, gpointer user_data, GDestroyNotify notify)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(self->freeze);

    self->output.user_data = user_data;
    self->output.notify = notify;
}

gpointer
j4status_section_get_output_user_data(J4statusSection *self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(self->freeze, NULL);

    return self->output.user_data;
}
