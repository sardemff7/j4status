AC_PREREQ([2.65])
AC_INIT([j4status],[0.1],[],[])

if test -d .git; then
    # Redefine PACKAGE_VERSION only to preserve VERSION for distdir
    PACKAGE_VERSION="${PACKAGE_VERSION} - `git describe --tags --always` (`git describe --tags --always --all | sed -e 's:heads/::'`)"
fi

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([main/src/j4status.c])
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
AM_GLIB_GNU_GETTEXT

AC_PROG_INSTALL

#
# Directories
#

# package-specific dirs
pkglibdir='${libdir}/'${PACKAGE}
pkgincludedir='${includedir}/'${PACKAGE}
pkgdatadir='${datadir}/'${PACKAGE}
AC_SUBST([pkglibdir])
AC_SUBST([pkgincludedir])
AC_SUBST([pkgdatadir])

# pkg-config dirs
PKG_INSTALLDIR

# aclocal dirs
aclocaldir='${datadir}/aclocal'
AC_SUBST([aclocaldir])

# j4status specific dirs
pluginsdir='${pkglibdir}/plugins'
AC_SUBST([pluginsdir])

# Common dependencies
glib_min_major="2"
glib_min_minor="40"
glib_min_version="${glib_min_major}.${glib_min_minor}"
gio_min_version="$glib_min_version"
gmodule_min_version="$glib_min_version"


AM_CFLAGS="-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
AM_CPPFLAGS=""
AM_LDFLAGS=""
AM_XSLTPROCFLAGS="--nonet --xinclude --stringparam man.output.quietly 1 --stringparam funcsynopsis.style ansi"
AM_DOCBOOK_CONDITIONS="installation"



GW_INIT([src/libgwater])
NK_INIT([src/libnkutils], [GLIB], [enum token/enum colour/string], [AM_DOCBOOK_CONDITIONS])

AC_CHECK_HEADERS([errno.h])

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

AC_ARG_ENABLE(systemd, AS_HELP_STRING([--enable-systemd], [Enable systemd activation support]), [], enable_systemd=no)
AC_ARG_WITH(systemduserunitdir, AS_HELP_STRING([--with-systemduserunitdir=DIR], [Directory for systemd user unit files]), [], with_systemduserunitdir=yes)
if test x$enable_systemd = xyes; then
    case "$with_systemduserunitdir" in
        no|"") AC_MSG_ERROR([*** You must define systemduserunitdir when compiling systemd support]) ;;
        yes) systemduserunitdir="`$PKG_CONFIG --variable=systemduserunitdir systemd`" ;;
        *) systemduserunitdir="$with_systemduserunitdir" ;;
    esac
    AC_CHECK_HEADERS([sys/socket.h])
    PKG_CHECK_MODULES(SYSTEMD, [libsystemd >= 209],, [PKG_CHECK_MODULES(SYSTEMD, [libsystemd-daemon])])
    AC_DEFINE([ENABLE_SYSTEMD], [1], [Enable systemd activation support])
    AC_SUBST(systemduserunitdir)
fi
AM_CONDITIONAL(ENABLE_SYSTEMD, test x$enable_systemd = xyes)

J4SP_INIT

J4SP_ADD_OUTPUT_PLUGIN(flat, [Flat], [always])
J4SP_ADD_OUTPUT_PLUGIN(debug, [Debug], [always])
J4SP_ADD_INPUT_PLUGIN(time, [Time], [always])
J4SP_ADD_INPUT_PLUGIN(file-monitor, [File monitor], [always])

yajl_min_version=2.0.0
J4SP_ADD_INPUT_OUTPUT_PLUGIN(i3bar, [i3bar JSON protocol], [yes], [
    AC_CHECK_HEADERS([signal.h])
    PKG_CHECK_MODULES(YAJL, [yajl >= $yajl_min_version])
])


J4SP_ADD_INPUT_PLUGIN(upower, [UPower], [yes], [
    PKG_CHECK_MODULES(UPOWER, [upower-glib])
])

J4SP_ADD_INPUT_PLUGIN(sensors, [libsensors], [yes], [
    LIBSENSORS_CFLAGS='-I${includedir}'
    LIBSENSORS_LIBS='-lsensors'
    AC_SEARCH_LIBS([sensors_init], [sensors],
        [
            AC_SUBST(LIBSENSORS_CFLAGS)
            AC_SUBST(LIBSENSORS_LIBS)
        ],
        [
            AC_MSG_ERROR([*** libsensors library is missing])
        ])
])

libpulse_min_version=0.9.16
J4SP_ADD_INPUT_PLUGIN(pulseaudio, [PULSEAUDIO], [yes], [
    PKG_CHECK_MODULES(PULSE, [libpulse >= $libpulse_min_version libpulse-mainloop-glib])
])

J4SP_ADD_INPUT_PLUGIN(mpd, [MPD], [no], [
    GW_CHECK_MPD
])

J4SP_ADD_INPUT_PLUGIN(systemd, [systemd], [no])

# Features defaulting to enable
# They more or less provide i3status features
if test x$enable_i3bar_input_output = xyes -a x$enable_nm_input = xyes -a x$enable_upower_input = xyes -a x$enable_sensors_input = xyes; then
    AM_DOCBOOK_CONDITIONS="${AM_DOCBOOK_CONDITIONS};enable_default_features"
fi

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
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_XSLTPROCFLAGS)
AC_SUBST(AM_DOCBOOK_CONDITIONS)

AM_SUBST_NOTMAKE(LIBS)
AC_SUBST(shrext_cmds)

AC_CONFIG_FILES([
    Makefile
    libj4status-plugin/pkgconfig/libj4status-plugin.pc
])
AC_OUTPUT

AC_MSG_RESULT([
    ${PACKAGE_NAME} ${PACKAGE_VERSION}

    UNIX socket support: ${have_gio_unix}
    systemd socket support: ${enable_systemd}
])

J4SP_SUMMARY

AC_MSG_RESULT([
dnl     Tests: $enable_tests
    Debug mode: $enable_debug
])
