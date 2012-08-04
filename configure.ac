AC_PREREQ([2.65])
AC_INIT([j4status],[0.1],[],[])

AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.11 subdir-objects dist-xz no-dist-gzip tar-ustar foreign])

AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_SED
AC_DISABLE_STATIC
LT_INIT
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([XSLTPROC], [xsltproc])

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

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= $glib_min_version])
AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [(G_ENCODE_VERSION(${glib_min_major},${glib_min_minor}))], [The lower GLib version supported])

PKG_CHECK_MODULES(GOBJECT, [gobject-2.0])

PKG_CHECK_MODULES(GTHREAD, [gthread-2.0])

PKG_CHECK_MODULES(GIO, [gio-2.0 >= $gio_min_version])

PKG_CHECK_MODULES(GMODULE, [gmodule-2.0 >= $gmodule_min_version])

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
if test x$enable_nm_input = xyes; then
    PKG_CHECK_MODULES(UPOWER, [upower-glib])
fi
AM_CONDITIONAL(ENABLE_UPOWER_INPUT, test x$enable_upower_input = xyes)


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
])
AC_OUTPUT
