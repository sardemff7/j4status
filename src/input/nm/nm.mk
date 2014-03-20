plugins_LTLIBRARIES += \
	nm.la \
	$(null)

nm_la_SOURCES = \
	src/input/nm/nm.c \
	$(null)

nm_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBNM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

nm_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

nm_la_LIBADD = \
	libj4status-plugin.la \
	$(LIBNM_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	man/j4status-nm.conf.5 \
	$(null)
