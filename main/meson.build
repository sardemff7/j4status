# Server
j4status_c_args = [
    '-DG_LOG_DOMAIN="j4status"',
]
j4status_deps = [
    libj4status_plugin,
    gthread,
    gio_platform,
    gio,
    gmodule,
    gobject,
    glib
]
if get_option('systemd')
    j4status_deps += dependency('libsystemd', version: '>= 209')
    j4status_c_args += '-DENABLE_SYSTEMD'
    docbook_conditions += 'enable_systemd'
endif

j4status = executable('j4status', [ config_h ] + files(
        'src/plugins.c',
        'src/plugins.h',
        'src/io.c',
        'src/io.h',
        'src/j4status.c',
        'src/j4status.h',
        'src/types.h',
    ),
    c_args: j4status_c_args,
    dependencies: j4status_deps,
    install: true,
)


man_pages += [ [ files('man/j4status.xml'), 'j4status.1' ] ]
man_pages += [ [ files('man/j4status.conf.xml'), 'j4status.conf.5' ] ]

if get_option('systemd')
    systemd = dependency('systemd')
    j4status_units = [
        'j4status@.socket',
        'j4status@.service',
    ]
    systemduserunit_install_dir = get_option('systemduserunitdir')
    if systemduserunit_install_dir == ''
        systemduserunit_install_dir = systemd.get_pkgconfig_variable('systemduserunitdir')
    endif
    eventd_user_units = []
    foreach u : j4status_units
        configure_file(
                input: files('units/@0@.in'.format(u)),
                output: u,
                configuration: other_conf,
                install_dir: systemduserunit_install_dir,
            )
    endforeach
endif
