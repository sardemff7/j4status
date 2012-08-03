plugins_LTLIBRARIES += \
	nm.la

nm_la_SOURCES = \
	src/input/nm/nm.c

nm_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBNM_CFLAGS) \
	$(GLIB_CFLAGS)

nm_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

nm_la_LIBADD = \
	libj4status.la \
	$(LIBNM_LIBS) \
	$(GLIB_LIBS)
