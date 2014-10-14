plugins_LTLIBRARIES += \
	pulseaudio.la \
	$(null)

man5_MANS += \
	input/pulseaudio/man/j4status-pulseaudio.conf.5 \
	$(null)


pulseaudio_la_SOURCES = \
	input/pulseaudio/src/pulseaudio.c \
	$(null)

pulseaudio_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-pulseaudio\" \
	$(null)

pulseaudio_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(PULSE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

pulseaudio_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

pulseaudio_la_LIBADD = \
	libj4status-plugin.la \
	$(PULSE_LIBS) \
	$(GLIB_LIBS) \
	$(null)
