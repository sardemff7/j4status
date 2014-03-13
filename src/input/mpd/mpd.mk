include libgwater/mpd.mk

plugins_LTLIBRARIES += \
	mpd.la \
	$(null)

mpd_la_SOURCES = \
	src/input/mpd/mpd.c \
	$(null)

mpd_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(MPD_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

mpd_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

mpd_la_LIBADD = \
	libj4status-plugin.la \
	$(MPD_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	man/j4status-mpd.conf.5 \
	$(null)
