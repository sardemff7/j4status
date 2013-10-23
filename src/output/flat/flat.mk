plugins_LTLIBRARIES += \
	flat.la \
	$(null)

flat_la_SOURCES = \
	src/output/flat/flat.c \
	$(null)

flat_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

flat_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output \
	$(null)

flat_la_LIBADD = \
	libj4status-plugin.la \
	$(GLIB_LIBS) \
	$(null)
