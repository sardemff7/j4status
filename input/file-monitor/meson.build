shared_library('file-monitor', [ config_h ] + files(
        'src/file-monitor.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-file-monitor"',
    ],
    dependencies: [ libj4status_plugin, gio, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)

man_pages += [ [ files('man/j4status-file-monitor.conf.xml'), 'j4status-file-monitor.conf.5' ] ]
