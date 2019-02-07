upower = dependency('upower-glib', required: get_option('upower'))

if upower.found()
    shared_library('upower', [ config_h ] + files(
            'src/upower.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-upower"',
        ],
        dependencies: [ upower, libj4status_plugin, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-upower.conf.xml'), 'j4status-upower.conf.5' ] ]
    docbook_conditions += 'enable_upower_input'
endif
