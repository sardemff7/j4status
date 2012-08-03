plugins_LTLIBRARIES += \
	flat.la

flat_la_SOURCES = \
	src/output/flat/flat.c

flat_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS)

flat_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output

flat_la_LIBADD = \
	$(GLIB_LIBS)
