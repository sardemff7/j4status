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
#include <gmodule.h>

#include <j4status-plugin.h>

#include "plugins.h"

#define PLUGINS_DATADIR DATADIR G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S "plugins"
#define PLUGINS_LIBDIR  LIBDIR  G_DIR_SEPARATOR_S PACKAGE_NAME G_DIR_SEPARATOR_S "plugins"

static GModule *
_j4status_plugins_try_dir(const gchar *dir, const gchar *file)
{
    if ( ! g_file_test(dir, G_FILE_TEST_IS_DIR) )
        return NULL;

    gchar *filename;
    filename = g_build_filename(dir, file, NULL);

    GModule *module = NULL;
    if ( g_file_test(filename, G_FILE_TEST_EXISTS) && ( ! g_file_test(filename, G_FILE_TEST_IS_DIR) ) )
    {
        module = g_module_open(filename, G_MODULE_BIND_LAZY|G_MODULE_BIND_LOCAL);
        if ( module == NULL )
            g_warning("Couldn't load module '%s': %s", file, g_module_error());
    }
    g_free(filename);
    return module;
}


static GModule *
_j4status_plugins_get_module(const gchar *file)
{
    GModule *module;

    const gchar *env_dir;
    env_dir = g_getenv("J4STATUS_PLUGINS_DIR");
    module = _j4status_plugins_try_dir(env_dir, file);
    if ( module != NULL )
        return module;

    gchar *dir;
    dir = g_build_filename(g_get_user_data_dir(), PACKAGE_NAME G_DIR_SEPARATOR_S "plugins", NULL);
    module = _j4status_plugins_try_dir(dir, file);
    g_free(dir);
    if ( module != NULL )
        return module;

    module = _j4status_plugins_try_dir(PLUGINS_DATADIR, file);
    if ( module != NULL )
        return module;
    module = _j4status_plugins_try_dir(PLUGINS_LIBDIR, file);
    if ( module != NULL )
        return module;

    return NULL;
}

J4statusOutputInitFunc
j4status_plugins_get_output_init_func(const gchar *name)
{
    if ( name == NULL )
        return j4status_plugins_get_output_init_func("flat");
    GModule *module;

    gchar *file;
    file = g_strconcat(name, "." G_MODULE_SUFFIX, NULL);
    module = _j4status_plugins_get_module(file);
    g_free(file);

    if ( ( module == NULL ) && ( ! g_str_equal(name, "flat") ) )
        return j4status_plugins_get_output_init_func("flat");

    J4statusOutputInitFunc func;

    if ( ! g_module_symbol(module, "j4status_output_init", (void **)&func) )
        return NULL;

    return func;
}

J4statusOutputFunc
j4status_plugins_get_output_func(const gchar *name)
{
    if ( name == NULL )
        return j4status_plugins_get_output_func("flat");
    GModule *module;

    gchar *file;
    file = g_strconcat(name, "." G_MODULE_SUFFIX, NULL);
    module = _j4status_plugins_get_module(file);
    g_free(file);

    if ( ( module == NULL ) && ( ! g_str_equal(name, "flat") ) )
        return j4status_plugins_get_output_func("flat");

    J4statusOutputFunc func;

    if ( ! g_module_symbol(module, "j4status_output", (void **)&func) )
        return NULL;

    if ( func == NULL )
        g_warning("Plugin '%s' must define j4status_output", name);

    return func;
}

typedef GList *(*J4statusInputFunc)();

static J4statusInputFunc
j4status_plugins_get_input_func(const gchar *name)
{
    if ( name == NULL )
        return NULL;
    GModule *module;

    gchar *file;
    file = g_strconcat(name, "." G_MODULE_SUFFIX, NULL);
    module = _j4status_plugins_get_module(file);
    g_free(file);

    if ( module == NULL )
        return NULL;

    J4statusInputFunc func;

    if ( ! g_module_symbol(module, "j4status_input", (void **)&func) )
        return NULL;

    if ( func == NULL )
        g_warning("Plugin '%s' must define j4status_input", name);

    return func;
}

GList *
j4status_plugins_get_sections(gchar **names)
{
    if ( names == NULL )
        return NULL;

    GList *sections = NULL;

    gchar **name;
    J4statusInputFunc func;
    for ( name = names ; *name != NULL ; ++name )
    {
        func = j4status_plugins_get_input_func(*name);
        if ( func != NULL )
            sections = g_list_concat(sections, func());
    }

    return sections;
}
