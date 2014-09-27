plugins_LTLIBRARIES += \
	flat.la \
	$(null)

flat_la_SOURCES = \
	output/flat/src/flat.c \
	$(null)

flat_la_CPPFLAGS = \
	-D G_LOG_DOMAIN=\"j4status-flat\" \
	$(null)

flat_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

flat_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output \
	$(null)

flat_la_LIBADD = \
	libj4status-plugin.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	output/flat/man/j4status-flat.conf.5 \
	$(null)
