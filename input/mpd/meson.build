libmpdclient = dependency('libmpdclient', required: get_option('mpd'))

if libmpdclient.found()
    libgwater_mpd = subproject('libgwater/mpd').get_variable('libgwater_mpd')

    shared_library('mpd', [ config_h ] + files(
            'src/mpd.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-mpd"',
        ],
        dependencies: [ libmpdclient, libgwater_mpd, libj4status_plugin, gio, gobject, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-mpd.conf.xml'), 'j4status-mpd.conf.5' ] ]
    docbook_conditions += 'enable_mpd_input'
endif
