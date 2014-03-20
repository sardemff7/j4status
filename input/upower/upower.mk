plugins_LTLIBRARIES += \
	upower.la \
	$(null)

upower_la_SOURCES = \
	input/upower/src/upower.c \
	$(null)

upower_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(UPOWER_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

upower_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

upower_la_LIBADD = \
	libj4status-plugin.la \
	$(UPOWER_LIBS) \
	$(GLIB_LIBS) \
	$(null)
