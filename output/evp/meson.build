
libeventc = dependency('libeventc', required: get_option('evp'))
libeventd = dependency('libeventd', required: get_option('evp'))

if libeventc.found() and libeventd.found()
    shared_library('evp', [ config_h ] + files(
            'src/evp.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-evp"',
        ],
        dependencies: [ libeventc, libeventd, libj4status_plugin, gio, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-evp.conf.xml'), 'j4status-evp.conf.5' ] ]
    docbook_conditions += 'enable_evp_output'
endif
