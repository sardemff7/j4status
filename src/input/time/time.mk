plugins_LTLIBRARIES += \
	time.la \
	$(null)

time_la_SOURCES = \
	src/input/time/time.c \
	$(null)

time_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

time_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

time_la_LIBADD = \
	libj4status-plugin.la \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	man/j4status-time.conf.5 \
	$(null)
