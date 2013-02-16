plugins_LTLIBRARIES += \
	file-monitor.la

file_monitor_la_SOURCES = \
	src/input/file-monitor/file-monitor.c

file_monitor_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS)
	$(GLIB_CFLAGS)

file_monitor_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

file_monitor_la_LIBADD = \
	libj4status-plugin.la \
	libj4status.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS)

man5_MANS += \
	man/j4status-file-monitor.conf.5
