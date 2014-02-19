plugins_LTLIBRARIES += \
	debug.la \
	$(null)

debug_la_SOURCES = \
	src/output/debug/debug.c \
	$(null)

debug_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

debug_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output \
	$(null)

debug_la_LIBADD = \
	libj4status-plugin.la \
	$(GLIB_LIBS) \
	$(null)
