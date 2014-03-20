plugins_LTLIBRARIES += \
	systemd.la \
	$(null)

systemd_la_SOURCES = \
	src/input/systemd/systemd.c \
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
	man/j4status-systemd.conf.5 \
	$(null)
