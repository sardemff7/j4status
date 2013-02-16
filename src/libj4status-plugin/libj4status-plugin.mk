# Plugin and core interfaces manipulation library

LIBJ4STATUS_PLUGIN_CURRENT=0
LIBJ4STATUS_PLUGIN_REVISION=0
LIBJ4STATUS_PLUGIN_AGE=0

lib_LTLIBRARIES += \
	libj4status-plugin.la

pkginclude_HEADERS += \
	include/j4status-plugin.h

pkgconfig_DATA += \
	src/libj4status-plugin/libj4status-plugin.pc


libj4status_plugin_la_SOURCES = \
	include/j4status-plugin.h \
	include/j4status-plugin-private.h \
	src/libj4status-plugin/core.c \
	src/libj4status-plugin/plugin.c

libj4status_plugin_la_CFLAGS = \
	$(AM_CFLAGS) \
	-D G_LOG_DOMAIN=\"libj4status-plugin\" \
	$(GLIB_CFLAGS)

libj4status_plugin_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(LIBJ4STATUS_PLUGIN_CURRENT):$(LIBJ4STATUS_PLUGIN_REVISION):$(LIBJ4STATUS_PLUGIN_AGE)

libj4status_plugin_la_LIBADD = \
	$(GLIB_LIBS)
