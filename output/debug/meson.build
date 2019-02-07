shared_library('debug', [ config_h ] + files(
        'src/debug.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-debug"',
    ],
    dependencies: [ libj4status_plugin, gio, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)
