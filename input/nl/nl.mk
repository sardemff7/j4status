include src/libgwater/nl.mk

plugins_LTLIBRARIES += \
	nl.la \
	$(null)

nl_la_SOURCES = \
	input/nl/src/nl.c \
	$(null)

nl_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-nl\" \
	$(null)

nl_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(NL_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

nl_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

nl_la_LIBADD = \
	libj4status-plugin.la \
	$(NL_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input/nl/man/j4status-nl.conf.5 \
	$(null)
