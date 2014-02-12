AC_PREREQ([2.65])
AC_INIT([j4status],[0.1],[],[])

AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.11 subdir-objects dist-xz no-dist-gzip tar-ustar foreign])

AM_SILENT_RULES([yes])

AC_ARG_VAR([XSLTPROC], [The xsltproc executable])

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_SED
AC_DISABLE_STATIC
LT_INIT
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([XSLTPROC], [xsltproc])
if test -z "$XSLTPROC"; then
    AC_MSG_ERROR([xsltproc not found])
fi

GETTEXT_PACKAGE=$PACKAGE_NAME
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext Package])
AC_SUBST(GETTEXT_PACKAGE)
dnl Not yet useful
dnl IT_PROG_INTLTOOL([0.40.0])

AC_PROG_INSTALL

# Common dependencies
glib_min_major="2"
glib_min_minor="28"
glib_min_version="${glib_min_major}.${glib_min_minor}"
gio_min_version="$glib_min_version"

# Server dependencies
gmodule_min_version="$glib_min_version"
libpulse_min_version="0.9.15"
libnotify_min_version="0.5.0"
gtk_min_version="3.0"

# Client dependencies
vala_min_version="0.12"


AM_CFLAGS="-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
AM_LDFLAGS="-no-undefined -Wl,--no-undefined"
AM_XSLTPROCFLAGS="--nonet --stringparam man.output.quietly 1 --stringparam funcsynopsis.style ansi"


if test -d .git; then
    PACKAGE_VERSION="${PACKAGE_VERSION} - `git describe --tags --always` (`git describe --tags --always --all | sed -e 's:heads/::'`)"
    AC_DEFINE_UNQUOTED([PACKAGE_VERSION], ["$PACKAGE_VERSION"], [Full version with git information])
fi

PKG_INSTALLDIR

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= $glib_min_version])
AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [(G_ENCODE_VERSION(${glib_min_major},${glib_min_minor}))], [The lower GLib version supported])

PKG_CHECK_MODULES(GOBJECT, [gobject-2.0])

PKG_CHECK_MODULES(GTHREAD, [gthread-2.0])

gio_unix=
PKG_CHECK_EXISTS([gio-unix-2.0 >= $gio_min_version], have_gio_unix=yes, have_gio_unix=no)
if test x$have_gio_unix = xyes; then
    gio_unix="gio-unix-2.0"
fi
PKG_CHECK_MODULES(GIO, [gio-2.0 >= $gio_min_version $gio_unix])

PKG_CHECK_MODULES(GMODULE, [gmodule-2.0 >= $gmodule_min_version])

AC_ARG_WITH(dbusinterfacesdir, AS_HELP_STRING([--with-dbusinterfacesdir=DIR], [Directory for D-Bus interfaces files]), [], with_dbusinterfacesdir=yes)
case "$with_dbusinterfacesdir" in
    no|"") AC_MSG_ERROR([*** You must define dbusinterfacesdir]) ;;
    yes) dbusinterfacesdir="`$PKG_CONFIG --variable=interfaces_dir dbus-1`" ;;
    *) dbusinterfacesdir="$with_dbusinterfacesdir" ;;
esac
AC_SUBST(dbusinterfacesdir)

AC_ARG_ENABLE(i3bar-output, AS_HELP_STRING([--disable-i3bar-output], [Disable i3bar JSON protocol support]), [], enable_i3bar_output=yes)
if test x$enable_i3bar_output = xyes; then
    PKG_CHECK_MODULES(YAJL, [yajl >= 2.0.0])
fi
AM_CONDITIONAL(ENABLE_I3BAR_OUTPUT, test x$enable_i3bar_output = xyes)

AC_ARG_ENABLE(nm-input, AS_HELP_STRING([--disable-nm-input], [Disable NetworkManager support]), [], enable_nm_input=yes)
if test x$enable_nm_input = xyes; then
    PKG_CHECK_MODULES(LIBNM, [libnm-util >= 0.9 libnm-glib >= 0.9])
fi
AM_CONDITIONAL(ENABLE_NM_INPUT, test x$enable_nm_input = xyes)

AC_ARG_ENABLE(upower-input, AS_HELP_STRING([--disable-upower-input], [Disable UPower support]), [], enable_upower_input=yes)
if test x$enable_upower_input = xyes; then
    PKG_CHECK_MODULES(UPOWER, [upower-glib])
fi
AM_CONDITIONAL(ENABLE_UPOWER_INPUT, test x$enable_upower_input = xyes)

AC_ARG_ENABLE(sensors-input, AS_HELP_STRING([--disable-sensors-input], [Disable libsensors support]), [], enable_sensors_input=yes)
if test x$enable_sensors_input = xyes; then
    LIBSENSORS_CFLAGS="-I/usr/include"
    LIBSENSORS_LIBS="-lsensors"
    AC_SEARCH_LIBS([sensors_init], [sensors],
        [
            AC_SUBST(LIBSENSORS_CFLAGS)
            AC_SUBST(LIBSENSORS_LIBS)
        ],
        [
            AC_MSG_ERROR([*** libsensors support requested but library is missing])
        ])
fi
AM_CONDITIONAL(ENABLE_SENSORS_INPUT, test x$enable_sensors_input = xyes)

AC_ARG_ENABLE(systemd-input, AS_HELP_STRING([--enable-systemd-input], [Enable systemd support]), [], enable_systemd_input=no)
if test x$enable_systemd_input = xyes; then
    PKG_CHECK_MODULES(SYSTEMD, [systemd])
    PKG_CHECK_MODULES(GLIB2_32, [glib-2.0 >= 2.32.0])
fi
AM_CONDITIONAL(ENABLE_SYSTEMD_INPUT, test x$enable_systemd_input = xyes)


#
# Tests
#
dnl AC_ARG_ENABLE(tests, AS_HELP_STRING([--enable-tests], [Build tests]), [], enable_tests=yes)
dnl AM_CONDITIONAL(ENABLE_TESTS, [test x$enable_tests = xyes])


# Debug flags
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [Enable debug]), [], enable_debug=no)
if test x$enable_debug = xyes; then
    AC_DEFINE([DEBUG], [1], [Activate debug mode])
fi
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_XSLTPROCFLAGS)

AM_SUBST_NOTMAKE(LIBS)
AC_SUBST(shrext_cmds)

AC_CONFIG_FILES([
    Makefile
    src/libj4status-plugin/libj4status-plugin.pc
])
AC_OUTPUT
