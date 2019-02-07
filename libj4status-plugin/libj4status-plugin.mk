# Plugin and core interfaces manipulation library

LIBJ4STATUS_PLUGIN_CURRENT=0
LIBJ4STATUS_PLUGIN_REVISION=0
LIBJ4STATUS_PLUGIN_AGE=0

AM_CPPFLAGS += \
	-I $(srcdir)/libj4status-plugin/include/ \
	$(null)

lib_LTLIBRARIES += \
	libj4status-plugin.la \
	$(null)

pkginclude_HEADERS += \
	libj4status-plugin/include/j4status-plugin.h \
	libj4status-plugin/include/j4status-plugin-input.h \
	libj4status-plugin/include/j4status-plugin-output.h \
	$(null)

dist_aclocal_DATA += \
	m4/j4status-plugins.m4 \
	$(null)

pkgconfig_DATA += \
	libj4status-plugin/pkgconfig/libj4status-plugin.pc \
	$(null)


libj4status_plugin_la_SOURCES = \
	libj4status-plugin/include/j4status-plugin.h \
	libj4status-plugin/include/j4status-plugin-input.h \
	libj4status-plugin/include/j4status-plugin-output.h \
	libj4status-plugin/include/j4status-plugin-private.h \
	libj4status-plugin/src/utils.c \
	libj4status-plugin/src/plugin.c \
	libj4status-plugin/src/core.c \
	libj4status-plugin/src/config.c \
	libj4status-plugin/src/section.c \
	$(null)

libj4status_plugin_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D J4STATUS_SYSCONFDIR=\"$(sysconfdir)\" \
	-D J4STATUS_LIBDIR=\"$(libdir)\" \
	-D J4STATUS_DATADIR=\"$(datadir)\" \
	$(null)

libj4status_plugin_la_CFLAGS = \
	$(AM_CFLAGS) \
	-D G_LOG_DOMAIN=\"libj4status-plugin\" \
	$(NKUTILS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

libj4status_plugin_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(LIBJ4STATUS_PLUGIN_CURRENT):$(LIBJ4STATUS_PLUGIN_REVISION):$(LIBJ4STATUS_PLUGIN_AGE) \
	$(null)

libj4status_plugin_la_LIBADD = \
	$(NKUTILS_LIBS) \
	$(GLIB_LIBS) \
	$(null)

include src/libnkutils/libnkutils.mk
