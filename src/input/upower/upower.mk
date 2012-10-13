plugins_LTLIBRARIES += \
	upower.la

upower_la_SOURCES = \
	src/input/upower/upower.c

upower_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(UPOWER_CFLAGS) \
	$(GLIB_CFLAGS)

upower_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

upower_la_LIBADD = \
	libj4status.la \
	$(UPOWER_LIBS) \
	$(GLIB_LIBS)

XSLTPROC_CONDITIONS += enable_upower_input
