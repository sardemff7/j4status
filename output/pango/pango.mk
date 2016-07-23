plugins_LTLIBRARIES += \
	pango.la \
	$(null)

pango_la_SOURCES = \
	output/pango/src/pango.c \
	$(null)

pango_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-pango\" \
	$(null)

pango_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

pango_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output \
	$(null)

pango_la_LIBADD = \
	libj4status-plugin.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	output/pango/man/j4status-pango.conf.5 \
	$(null)
