shared_library('time', [ config_h ] + files(
        'src/time.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-time"',
    ],
    dependencies: [ libj4status_plugin, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)

man_pages += [ [ files('man/j4status-time.conf.xml'), 'j4status-time.conf.5' ] ]
