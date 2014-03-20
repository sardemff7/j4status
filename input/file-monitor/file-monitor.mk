plugins_LTLIBRARIES += \
	file-monitor.la \
	$(null)

file_monitor_la_SOURCES = \
	input/file-monitor/src/file-monitor.c \
	$(null)

file_monitor_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(null)
	$(GLIB_CFLAGS) \
	$(null)

file_monitor_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

file_monitor_la_LIBADD = \
	libj4status-plugin.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input/file-monitor/man/j4status-file-monitor.conf.5 \
	$(null)
