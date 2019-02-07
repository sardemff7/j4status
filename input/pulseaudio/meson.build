libpulse_min_version = '0.9.16'

libpulse = dependency('libpulse', version: '>=@0@'.format(libpulse_min_version), required: get_option('pulseaudio'))
libpulse_glib = dependency('libpulse-mainloop-glib', required: get_option('pulseaudio'))

if libpulse.found() and libpulse_glib.found()
    shared_library('pulseaudio', [ config_h ] + files(
            'src/pulseaudio.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-pulseaudio"',
        ],
        dependencies: [ libpulse_glib, libpulse, libj4status_plugin, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-pulseaudio.conf.xml'), 'j4status-pulseaudio.conf.5' ] ]
    docbook_conditions += 'enable_pulseaudio_input'
endif
