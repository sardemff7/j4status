include src/libgwater/mpd.mk

plugins_LTLIBRARIES += \
	mpd.la \
	$(null)

mpd_la_SOURCES = \
	input/mpd/src/mpd.c \
	$(null)

mpd_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-mpd\" \
	$(null)

mpd_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GW_MPD_CFLAGS) \
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
	$(GW_MPD_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input/mpd/man/j4status-mpd.conf.5 \
	$(null)
