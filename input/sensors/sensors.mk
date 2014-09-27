plugins_LTLIBRARIES += \
	sensors.la \
	$(null)

sensors_la_SOURCES = \
	input/sensors/src/sensors.c \
	$(null)

sensors_la_CPPFLAGS = \
	-D G_LOG_DOMAIN=\"j4status-sensors\" \
	$(null)

sensors_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBSENSORS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

sensors_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

sensors_la_LIBADD = \
	libj4status-plugin.la \
	$(LIBSENSORS_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input/sensors/man/j4status-sensors.conf.5 \
	$(null)
