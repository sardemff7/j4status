shared_library('flat', [ config_h ] + files(
        'src/flat.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-flat"',
    ],
    dependencies: [ libj4status_plugin, gio, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)

man_pages += [ [ files('man/j4status-flat.conf.xml'), 'j4status-flat.conf.5' ] ]
