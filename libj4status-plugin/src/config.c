/*
 * j4status - Status line generator
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

#include <nkutils-enum.h>

#define CONFIG_SYSCONFFILE SYSCONFDIR G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S "config"
#define CONFIG_DATAFILE    DATADIR    G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S "config"
#define CONFIG_LIBFILE     LIBDIR     G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S "config"

static GKeyFile *
_j4status_config_try_dir(const gchar *filename, const gchar *section)
{
    GKeyFile *key_file = NULL;
    if ( g_file_test(filename, G_FILE_TEST_EXISTS) && ( ! g_file_test(filename, G_FILE_TEST_IS_DIR) ) )
    {
        GError *error = NULL;
        key_file = g_key_file_new();
        if ( ! g_key_file_load_from_file(key_file, filename, 0, &error) )
        {
            g_key_file_free(key_file);
            g_warning("Couldn't load key_file '%s': %s", filename, error->message);
            g_clear_error(&error);
            return NULL;
        }
        if ( g_key_file_has_group(key_file, section) )
            return key_file;
        else
        {
            g_key_file_free(key_file);
            return NULL;
        }
    }
    return key_file;
}


J4STATUS_EXPORT GKeyFile *
j4status_config_get_key_file(const gchar *section)
{
    GKeyFile *key_file;
    gchar *file = NULL;

    const gchar *env_file;
    env_file = g_getenv("J4STATUS_CONFIG_FILE");
    if ( env_file != NULL )
    {
        if ( strchr(env_file, G_DIR_SEPARATOR) == NULL )
            env_file = file = g_build_filename(g_get_user_config_dir(), PACKAGE_NAME, env_file, NULL);
        key_file = _j4status_config_try_dir(env_file, section);
        g_free(file);
        if ( key_file != NULL )
            return key_file;
    }

    file = g_build_filename(g_get_user_config_dir(), PACKAGE_NAME G_DIR_SEPARATOR_S "config", NULL);
    key_file = _j4status_config_try_dir(file, section);
    g_free(file);
    if ( key_file != NULL )
        return key_file;

    key_file = _j4status_config_try_dir(CONFIG_SYSCONFFILE, section);
    if ( key_file != NULL )
        return key_file;
    key_file = _j4status_config_try_dir(CONFIG_DATAFILE, section);
    if ( key_file != NULL )
        return key_file;
    key_file = _j4status_config_try_dir(CONFIG_LIBFILE, section);
    if ( key_file != NULL )
        return key_file;

    return NULL;
}

J4STATUS_EXPORT gboolean
j4status_config_key_file_get_enum(GKeyFile *key_file, const gchar *group_name, const gchar *key, const gchar * const *values, guint64 size, guint64 *value)
{
    gchar *string;
    string = g_key_file_get_string(key_file, group_name, key, NULL);
    if ( string == NULL )
        return FALSE;

    gboolean r;

    r = nk_enum_parse(string, values, size, TRUE, FALSE, value);
    g_free(string);

    return r;
}

J4STATUS_EXPORT GHashTable *
j4status_config_key_file_get_actions(GKeyFile *key_file, const gchar *group_name, const gchar * const *actions, guint64 size)
{
    gchar **strings;
    strings = g_key_file_get_string_list(key_file, group_name, "Actions", NULL, NULL);
    if ( strings == NULL )
        return NULL;

    GHashTable *table;
    table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    gchar **string;
    for ( string  = strings ; *string != NULL ; ++string )
    {
        gchar *id = *string;
        gchar *action;
        guint64 value;

        action = g_utf8_strchr(*string, -1, ' ');
        if ( action == NULL )
            goto next;
        *action++ = '\0';

        if ( nk_enum_parse(action, actions, size, TRUE, FALSE, &value) )
            g_hash_table_insert(table, g_strdup(id), GUINT_TO_POINTER(value));

    next:
        g_free(*string);
    }
    g_free(strings);

    if ( g_hash_table_size(table) < 1 )
    {
        g_hash_table_unref(table);
        return NULL;
    }

    return table;
}
