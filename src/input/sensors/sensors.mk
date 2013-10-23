plugins_LTLIBRARIES += \
	sensors.la \
	$(null)

sensors_la_SOURCES = \
	src/input/sensors/sensors.c \
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
	libj4status.la \
	$(LIBSENSORS_LIBS) \
	$(GLIB_LIBS) \
	$(null)

XSLTPROC_CONDITIONS += enable_sensors_input

man5_MANS += \
	man/j4status-sensors.conf.5 \
	$(null)
