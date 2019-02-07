libsensors = c_compiler.find_library('sensors', required: get_option('sensors'))

if libsensors.found()
    shared_library('sensors', [ config_h ] + files(
            'src/sensors.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-sensors"',
        ],
        dependencies: [ libsensors, libj4status_plugin, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-sensors.conf.xml'), 'j4status-sensors.conf.5' ] ]
    docbook_conditions += 'enable_sensors_input'
endif
