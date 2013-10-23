# Server
noinst_LTLIBRARIES += \
	libj4status.la \
	$(null)

libj4status_la_SOURCES = \
	include/libj4status-config.h \
	src/libj4status/config.c \
	$(null)

libj4status_la_CFLAGS = \
	$(AM_CFLAGS) \
	-D SYSCONFDIR=\"$(sysconfdir)\" \
	-D LIBDIR=\"$(libdir)\" \
	-D DATADIR=\"$(datadir)\" \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

libj4status_la_LIBADD = \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)
