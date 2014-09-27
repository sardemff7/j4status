plugins_LTLIBRARIES += \
	systemd.la \
	$(null)

systemd_la_SOURCES = \
	input/systemd/src/systemd.c \
	$(null)

systemd_la_CPPFLAGS = \
	-D G_LOG_DOMAIN=\"j4status-systemd\" \
	$(null)

systemd_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

systemd_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

systemd_la_LIBADD = \
	libj4status-plugin.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	input/systemd/man/j4status-systemd.conf.5 \
	$(null)
