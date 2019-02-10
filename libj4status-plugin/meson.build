# Plugin and core interfaces manipulation library

libj4status_plugin_inc = include_directories('include')
libj4status_plugin_dep = [
    libnkutils,
    glib,
]

libj4status_plugin_sources = files(
    'src/utils.c',
    'src/plugin.c',
    'src/core.c',
    'src/config.c',
    'src/section.c',

)

libj4status_plugin_lib = library('j4status-plugin', [ config_h ] + files(
    'include/j4status-plugin-private.h',
    ) + libj4status_plugin_sources,
	c_args: [
		'-DG_LOG_DOMAIN="libj4status-plugins"',
	],
	dependencies: libj4status_plugin_dep,
	version: '0.0.0',
	include_directories: libj4status_plugin_inc,
	install: true,
)

libj4status_plugin_headers = files(
    'include/j4status-plugin.h',
    'include/j4status-plugin-input.h',
    'include/j4status-plugin-output.h',
)

install_headers(libj4status_plugin_headers,
    subdir: meson.project_name(),
)

libj4status_plugin = declare_dependency(link_with: libj4status_plugin_lib, include_directories: libj4status_plugin_inc, dependencies: libj4status_plugin_dep)

pkgconfig.generate(libj4status_plugin_lib,
    filebase: 'libj4status-plugin',
    name: 'libj4status-plugin',
    version: meson.project_version(),
    description: 'Library to implement a j4status plugin',
    url: 'https://sardemff7.github.io/j4status/',
    subdirs: meson.project_name(),
    variables: [
        'pluginsdir=@0@'.format(join_paths('${prefix}', plugins_install_dir)),
    ],
)
