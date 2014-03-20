plugins_LTLIBRARIES += \
	i3bar.la \
	$(null)

i3bar_la_SOURCES = \
	src/input-output/i3bar/input.c \
	src/input-output/i3bar/output.c \
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
	man/j4status-i3bar.conf.5 \
	$(null)
