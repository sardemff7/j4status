plugins_LTLIBRARIES += \
	sensors.la

sensors_la_SOURCES = \
	src/input/sensors/sensors.c

sensors_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBSENSORS_CFLAGS) \
	$(GLIB_CFLAGS)

sensors_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

sensors_la_LIBADD = \
	libj4status.la \
	$(LIBSENSORS_LIBS) \
	$(GLIB_LIBS)

man5_MANS += \
	man/j4status-sensors.conf.5
