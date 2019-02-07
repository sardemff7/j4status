yajl_min_version='2.0.0'

yajl = dependency('yajl', version: '>=@0@'.format(yajl_min_version), required: get_option('i3bar'))

if yajl.found()
    shared_library('i3bar', [ config_h ] + files(
            'src/input.c',
            'src/output.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-i3bar"',
        ],
        dependencies: [ yajl, libj4status_plugin, gio_platform, gio, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-i3bar.conf.xml'), 'j4status-i3bar.conf.5' ] ]
    docbook_conditions += 'enable_i3bar_input_output'
endif
