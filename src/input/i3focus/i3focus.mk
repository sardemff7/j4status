plugins_LTLIBRARIES += \
	i3focus.la \
	$(null)

i3focus_la_SOURCES = \
	src/input/i3focus/i3focus.c \
	$(null)

i3focus_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(I3IPC_GLIB_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

i3focus_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

i3focus_la_LIBADD = \
	libj4status-plugin.la \
	libj4status.la \
	$(I3IPC_GLIB_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)
