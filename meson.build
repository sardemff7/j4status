project('j4status', 'c',
    version: '0.1',
    meson_version: '>=0.47.0',
    license: [ 'GPL3+', 'LGPL3+', 'MIT' ],
    default_options: [
        'c_std=gnu11',
        'warning_level=2',
    ],
)

is_unix = host_machine.system() != 'windows'
is_windows = not is_unix
pkgconfig = import('pkgconfig')
gnome = import('gnome')

glib_min_major='2'
glib_min_minor='40'
glib_min_version='.'.join([glib_min_major, glib_min_minor])
glib = dependency('glib-2.0', version: '>= @0@'.format(glib_min_version))
gobject = dependency('gobject-2.0')
gio = dependency('gio-2.0')
if is_unix
    gio_platform = dependency('gio-unix-2.0')
else
    gio_platform = dependency('gio-windows-2.0')
endif
gthread = dependency('gthread-2.0')
gmodule = dependency('gmodule-2.0')
headers = [
    'string.h',
    'unistd.h',
    'locale.h',
    'errno.h',
    'signal.h',
]
if is_unix
    headers += [
        'sys/socket.h',
    ]
else
    headers += [
        'windows.h',
    ]
endif
c_compiler = meson.get_compiler('c')
foreach h : headers
    if not c_compiler.has_header(h)
        error('Header @0@ was not found, but is required'.format(h))
    endif
endforeach

plugins_install_dir = join_paths(get_option('libdir'), meson.project_name(), 'plugins')

header_conf = configuration_data()
other_conf = configuration_data()
header_conf.set_quoted('PACKAGE_NAME', meson.project_name())
header_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
header_conf.set_quoted('MODULES_VERSION', meson.project_version())
header_conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
other_conf.set('PACKAGE_NAME', meson.project_name())
other_conf.set('PACKAGE_VERSION', meson.project_version())
other_conf.set('VERSION', meson.project_version())

if is_unix
    header_conf.set('J4STATUS_EXPORT', '__attribute__((visibility("default")))')
else
    header_conf.set('J4STATUS_EXPORT', '__declspec(dllexport)')
    header_conf.set('_WIN32_WINNT', '_WIN32_WINNT_WIN7')
    header_conf.set('NTDDI_VERSION', 'NTDDI_WIN7')
endif
header_conf.set('GLIB_VERSION_MIN_REQUIRED', '(G_ENCODE_VERSION(@0@,@1@))'.format(glib_min_major, glib_min_minor))
header_conf.set('G_LOG_USE_STRUCTURED', true)

header_conf.set_quoted('J4STATUS_SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
header_conf.set_quoted('J4STATUS_BINDIR', join_paths(get_option('prefix'), get_option('bindir')))
header_conf.set_quoted('J4STATUS_LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
header_conf.set_quoted('J4STATUS_DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
header_conf.set_quoted('J4STATUS_LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
other_conf.set('prefix', get_option('prefix'))
other_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))
other_conf.set('datadir', join_paths(get_option('prefix'), get_option('datadir')))


header_conf.set('J4STATUS_DEBUG_OUTPUT', get_option('debug-output'))

config_h = configure_file(output: 'config.h', configuration: header_conf)
config_ent = configure_file(input: files('src/config.ent.in'), output: 'config.ent', configuration: other_conf)

add_project_arguments(
    '-fvisibility=hidden',
    '-I@0@'.format(meson.build_root()),
    '-I@0@/src'.format(meson.source_root()),
    language: 'c'
)
flags = [
    '-Wformat=2',
    '-Wno-unused-parameter',
]
foreach f : flags
    if c_compiler.has_argument(f)
        add_project_arguments(f, language: 'c')
    endif
endforeach

man_pages = []
docbook_conditions = [ 'installation' ]

nk_options = [
    'enum=true',
    'token=true',
    'colour=true',
    'git-work-tree=@0@'.format(meson.source_root()),
]
nk = subproject('libnkutils', default_options: nk_options)
nk_subproject_options = nk.get_variable('nk_options')
foreach o : nk_options + nk_subproject_options
    if not nk_options.contains(o) or not nk_subproject_options.contains(o)
        error('You must not change libnkutils options @0@ != @1@'.format('|'.join(nk_options), '|'.join(nk_subproject_options)))
    endif
endforeach
libnkutils = nk.get_variable('libnkutils')
nkutils_xsltpaths = nk.get_variable('nkutils_xsltpaths')
nkutils_manfiles = nk.get_variable('nkutils_manfiles')
nkutils_mandepends = nk.get_variable('nkutils_mandepends')
docbook_conditions += nk.get_variable('nkutils_docbook_conditions')

subdir('libj4status-plugin')
subdir('main')

subdir('output/debug')
subdir('output/flat')
subdir('output/pango')
subdir('output/evp')

subdir('input/time')
subdir('input/file-monitor')
subdir('input/systemd')
subdir('input/upower')
subdir('input/sensors')
subdir('input/nl')
subdir('input/pulseaudio')
subdir('input/mpd')

subdir('input-output/i3bar')

xsltproc = [
    find_program('xsltproc'),
    '-o', '@OUTDIR@',
    '--nonet', '--xinclude',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'profile.condition', ';'.join(docbook_conditions),
]
foreach p : [
    join_paths(meson.current_source_dir(), 'src'),
    meson.current_build_dir(),
] + nkutils_xsltpaths
    xsltproc += [ '--path', p ]
endforeach
xsltproc += [
    'http://docbook.sourceforge.net/release/xsl/current/manpages/profile-docbook.xsl',
    '@INPUT@'
]
man_input = []
man_output = []
foreach m : man_pages
    s = m[1].split('.')
    section = 'man' + s[s.length() - 1]
    custom_target(m[1],
        input: m[0],
        output: m[1],
        command: xsltproc,
        depend_files: [ config_ent ] + nkutils_manfiles,
        depends: nkutils_mandepends,
        build_by_default: true,
        install: true,
        install_dir: join_paths(get_option('mandir'), section)
    )
endforeach
