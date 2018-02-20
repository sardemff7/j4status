plugins_LTLIBRARIES += \
	evp.la \
	$(null)

evp_la_SOURCES = \
	output/evp/src/evp.c \
	$(null)

evp_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-D G_LOG_DOMAIN=\"j4status-evp\" \
	$(null)

evp_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(EVENTC_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

evp_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_output \
	$(null)

evp_la_LIBADD = \
	libj4status-plugin.la \
	$(EVENTC_LIBS) \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	output/evp/man/j4status-evp.conf.5 \
	$(null)
