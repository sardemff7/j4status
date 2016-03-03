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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <glib.h>
#include <sensors/sensors.h>

#include <j4status-plugin-input.h>

#define MAX_CHIP_NAME_SIZE 256

struct _J4statusPluginContext {
    J4statusCoreInterface *core;
    GList *sections;
    struct {
        gboolean show_details;
    } config;
    gboolean started;
};

typedef struct {
    J4statusSection *section;

    const sensors_chip_name *chip;
    const sensors_feature *feature;
    const sensors_subfeature *input;
    const sensors_subfeature *max;
    const sensors_subfeature *crit;
} J4statusSensorsFeature;

static void
_j4status_sensors_feature_temp_update(J4statusPluginContext *context, J4statusSensorsFeature *feature)
{
    double curr;
    sensors_get_value(feature->chip, feature->input->number, &curr);

    double high = -1;
    if ( feature->max != NULL )
        sensors_get_value(feature->chip, feature->max->number, &high);

    double crit = -1;
    if ( feature->crit != NULL )
        sensors_get_value(feature->chip, feature->crit->number, &crit);

    J4statusState state;

    if ( ( high > 0 ) && ( curr > high ) )
        state = J4STATUS_STATE_BAD;
    else
        state = J4STATUS_STATE_GOOD;

    if ( ( crit > 0 ) && ( curr > crit ) )
        state |= J4STATUS_STATE_URGENT;

    if ( ( ! context->started ) && ( ( state & J4STATUS_STATE_URGENT ) == 0 ) )
        return;

    j4status_section_set_state(feature->section, state);

    if ( ! context->config.show_details )
        high = crit = -1;

    gchar *value;
    if ( ( high > 0 ) && ( crit > 0 ) )
        value = g_strdup_printf("%+.1f°C (high = %+.1f°C, crit = %+.1f°C)", curr, high, crit);
    else if ( high > 0 )
        value = g_strdup_printf("%+.1f°C (high = %+.1f°C)", curr, high);
    else if ( crit > 0 )
        value = g_strdup_printf("%+.1f°C (crit = %+.1f°C)", curr, crit);
    else
        value = g_strdup_printf("%+.1f°C", curr);
    j4status_section_set_value(feature->section, value);
}

static void
_j4status_sensors_feature_fan_update(J4statusPluginContext *context, J4statusSensorsFeature *feature)
{
    double curr;
    sensors_get_value(feature->chip, feature->input->number, &curr);

    double high = -1;
    if ( feature->max != NULL )
        sensors_get_value(feature->chip, feature->max->number, &high);

    J4statusState state;

    if ( ( high > 0 ) && ( curr > high ) )
        state = J4STATUS_STATE_BAD;
    else
        state = J4STATUS_STATE_GOOD;

    j4status_section_set_state(feature->section, state);

    if ( ! context->config.show_details )
        high = -1;

    gchar *value;
    if ( high > 0 )
        value = g_strdup_printf("%.0frpm (high = %.0frpm)", curr, high);
    else
        value = g_strdup_printf("%.0frpm", curr);

    j4status_section_set_value(feature->section, value);
}

static gboolean
_j4status_sensors_update(gpointer user_data)
{
    J4statusPluginContext *context = user_data;

    GList *feature_;
    for ( feature_ = context->sections ; feature_ != NULL ; feature_ = g_list_next(feature_) )
    {
        J4statusSensorsFeature *feature = feature_->data;
        switch ( feature->feature->type )
        {
        case SENSORS_FEATURE_TEMP:
            _j4status_sensors_feature_temp_update(context, feature);
        break;
        case SENSORS_FEATURE_FAN:
            _j4status_sensors_feature_fan_update(context, feature);
        break;
        default:
            g_return_val_if_reached(TRUE);
        }
    }

    return TRUE;
}

static void
_j4status_sensors_feature_free(gpointer data)
{
    J4statusSensorsFeature *feature = data;

    j4status_section_free(feature->section);

    g_free(feature);
}

// Not re-entrant/thread-safe
static const char *
_j4status_sensors_get_feature_name(const sensors_chip_name *chip, const sensors_feature *feature)
{
    int n;
    static char name[MAX_CHIP_NAME_SIZE + NAME_MAX + 1];
    n = sensors_snprintf_chip_name(name, MAX_CHIP_NAME_SIZE, chip);
    name[n++] = '/';
    strcpy(name + n, feature->name);

    return name;
}

static void
_j4status_sensors_add_feature_fan(J4statusPluginContext *context, const sensors_chip_name *chip, const sensors_feature *feature)
{
    const char *name = _j4status_sensors_get_feature_name(chip, feature);

    const sensors_subfeature *input;

    input = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_FAN_INPUT);
    if ( input == NULL )
    {
        g_warning("No fan input on chip '%s', skipping", name);
        return;
    }

    J4statusSensorsFeature *sensor_feature;
    sensor_feature = g_new0(J4statusSensorsFeature, 1);
    sensor_feature->section = j4status_section_new(context->core);
    sensor_feature->chip = chip;
    sensor_feature->feature = feature;
    sensor_feature->input = input;
    sensor_feature->max = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_FAN_MAX);
    sensor_feature->crit = NULL;

    char *label;
    label = sensors_get_label(chip, feature);

    gint64 max_width = strlen("10000rpm");
    if ( ( context->config.show_details ) && ( sensor_feature->max != NULL ) )
    {
        max_width = strlen("10000rpm (high = 10000rpm)");
    }

    j4status_section_set_name(sensor_feature->section, "sensors");
    j4status_section_set_instance(sensor_feature->section, name);
    j4status_section_set_label(sensor_feature->section, label);
    j4status_section_set_max_width(sensor_feature->section, -max_width);

    free(label);

    if ( j4status_section_insert(sensor_feature->section) )
        context->sections = g_list_prepend(context->sections, sensor_feature);
    else
        _j4status_sensors_feature_free(sensor_feature);
}

static void
_j4status_sensors_add_feature_temp(J4statusPluginContext *context, const sensors_chip_name *chip, const sensors_feature *feature)
{
    const char *name = _j4status_sensors_get_feature_name(chip, feature);

    const sensors_subfeature *input;

    input = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
    if ( input == NULL )
    {
        g_warning("No temperature input on chip '%s', skipping", name);
        return;
    }

    J4statusSensorsFeature *sensor_feature;
    sensor_feature = g_new0(J4statusSensorsFeature, 1);
    sensor_feature->section = j4status_section_new(context->core);
    sensor_feature->chip = chip;
    sensor_feature->feature = feature;
    sensor_feature->input = input;
    sensor_feature->max = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_MAX);
    sensor_feature->crit = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_CRIT);

    char *label;
    label = sensors_get_label(chip, feature);

    gint64 max_width = strlen("+100.0*C");
    if ( context->config.show_details )
    {
        if ( ( sensor_feature->crit != NULL ) && ( sensor_feature->max != NULL ) )
            max_width += strlen(" (high = +100.0°C, crit = +100.0°C)");
        else if ( sensor_feature->crit != NULL )
            max_width += strlen(" (crit = +100.0°C)");
        else if ( sensor_feature->max != NULL )
            max_width += strlen(" (high = +100.0°C)");
    }

    j4status_section_set_name(sensor_feature->section, "sensors");
    j4status_section_set_instance(sensor_feature->section, name);
    j4status_section_set_label(sensor_feature->section, label);
    j4status_section_set_max_width(sensor_feature->section, -max_width);

    free(label);

    if ( j4status_section_insert(sensor_feature->section) )
        context->sections = g_list_prepend(context->sections, sensor_feature);
    else
        _j4status_sensors_feature_free(sensor_feature);
}

static void
_j4status_sensors_add_sensors(J4statusPluginContext *context, const sensors_chip_name *match)
{
    const sensors_chip_name *chip;
    const sensors_feature *feature;
    int chip_nr = 0;
    while ( ( chip = sensors_get_detected_chips(match, &chip_nr) ) != NULL )
    {
        int feature_nr = 0;
        while ( ( feature = sensors_get_features(chip, &feature_nr) ) != NULL )
        {
            switch (feature->type)
            {
            case SENSORS_FEATURE_TEMP:
                _j4status_sensors_add_feature_temp(context, chip, feature);
            break;
            case SENSORS_FEATURE_FAN:
                _j4status_sensors_add_feature_fan(context, chip, feature);
            break;
            default:
                continue;
            }
        }
    }
}

static void _j4status_sensors_uninit(J4statusPluginContext *context);

static J4statusPluginContext *
_j4status_sensors_init(J4statusCoreInterface *core)
{
    gchar **sensors = NULL;
    gboolean show_details = FALSE;
    guint64 interval = 0;

    if ( sensors_init(NULL) != 0 )
        return NULL;

    GKeyFile *key_file;
    key_file = j4status_config_get_key_file("Sensors");
    if ( key_file != NULL )
    {
        sensors = g_key_file_get_string_list(key_file, "Sensors", "Sensors", NULL, NULL);
        show_details = g_key_file_get_boolean(key_file, "Sensors", "ShowDetails", NULL);
        interval = g_key_file_get_uint64(key_file, "Sensors", "Interval", NULL);
        g_key_file_free(key_file);
    }

    J4statusPluginContext *context;
    context = g_new0(J4statusPluginContext, 1);
    context->core = core;

    context->config.show_details = show_details;


    if ( sensors == NULL )
        _j4status_sensors_add_sensors(context, NULL);
    else
    {
        sensors_chip_name chip;
        gchar **sensor;
        for ( sensor = sensors ; *sensor != NULL ; ++sensor )
        {
            if ( sensors_parse_chip_name(*sensor, &chip) != 0 )
                continue;
            _j4status_sensors_add_sensors(context, &chip);
            sensors_free_chip_name(&chip);
        }
    }
    g_strfreev(sensors);

    if ( context->sections == NULL )
    {
        g_message("Missing configuration: No sensor to monitor, aborting");
        _j4status_sensors_uninit(context);
        return NULL;
    }

    g_timeout_add_seconds(MAX(2, interval), _j4status_sensors_update, context);

    return context;
}

static void
_j4status_sensors_uninit(J4statusPluginContext *context)
{
    g_list_free_full(context->sections, _j4status_sensors_feature_free);

    g_free(context);

    sensors_cleanup();
}

static void
_j4status_sensors_start(J4statusPluginContext *context)
{
    context->started = TRUE;
    _j4status_sensors_update(context);
}

static void
_j4status_sensors_stop(J4statusPluginContext *context)
{
    context->started = FALSE;
}

void
j4status_input_plugin(J4statusInputPluginInterface *interface)
{
    libj4status_input_plugin_interface_add_init_callback(interface, _j4status_sensors_init);
    libj4status_input_plugin_interface_add_uninit_callback(interface, _j4status_sensors_uninit);

    libj4status_input_plugin_interface_add_start_callback(interface, _j4status_sensors_start);
    libj4status_input_plugin_interface_add_stop_callback(interface, _j4status_sensors_stop);
}
