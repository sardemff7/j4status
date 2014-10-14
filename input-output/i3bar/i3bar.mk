plugins_LTLIBRARIES += \
	i3bar.la \
	$(null)

i3bar_la_SOURCES = \
	input-output/i3bar/src/input.c \
	input-output/i3bar/src/output.c \
	$(null)

i3bar_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-i3bar\" \
	$(null)

i3bar_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(YAJL_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

i3bar_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex 'j4status_(in|out)put' \
	$(null)

i3bar_la_LIBADD = \
	libj4status-plugin.la \
	$(YAJL_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input-output/i3bar/man/j4status-i3bar.conf.5 \
	$(null)
