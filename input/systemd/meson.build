shared_library('systemd', [ config_h ] + files(
        'src/systemd.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-systemd"',
    ],
    dependencies: [ libj4status_plugin, gio, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)

man_pages += [ [ files('man/j4status-systemd.conf.xml'), 'j4status-systemd.conf.5' ] ]
