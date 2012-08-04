plugins_LTLIBRARIES += \
	time.la

man5_MANS += \
	man/j4status-time.conf.5

time_la_SOURCES = \
	src/input/time/time.c

time_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS)

time_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

time_la_LIBADD = \
	libj4status.la \
	$(GLIB_LIBS)
