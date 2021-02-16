
libnl_route = dependency('libnl-route-3.0', required: get_option('nl'))
libnl_genl = dependency('libnl-genl-3.0', required: get_option('nl'))

if libnl_route.found() and libnl_genl.found()
    libgwater_nl = subproject('libgwater').get_variable('libgwater_nl')
    foreach h : [ 'sys/socket.h', 'linux/if_arp.h', 'linux/nl80211.h' ]
        if not c_compiler.has_header(h)
            error('Header @0@ was not found, but is required'.format(h))
        endif
    endforeach

    shared_library('nl', [ config_h ] + files(
            'src/nl.c',
        ),
        c_args: [
            '-DG_LOG_DOMAIN="j4status-nl"',
        ],
        dependencies: [ libnl_route, libnl_genl, libgwater_nl, libj4status_plugin, gio_platform, gio, gobject, glib ],
        name_prefix: '',
        install: true,
        install_dir: plugins_install_dir,
    )

    man_pages += [ [ files('man/j4status-nl.conf.xml'), 'j4status-nl.conf.5' ] ]
    docbook_conditions += 'enable_nl_input'
endif
