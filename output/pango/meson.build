shared_library('pango', [ config_h ] + files(
        'src/pango.c',
    ),
    c_args: [
        '-DG_LOG_DOMAIN="j4status-pango"',
    ],
    dependencies: [ libj4status_plugin, gio, glib ],
    name_prefix: '',
    install: true,
    install_dir: plugins_install_dir,
)

man_pages += [ [ files('man/j4status-pango.conf.xml'), 'j4status-pango.conf.5' ] ]
