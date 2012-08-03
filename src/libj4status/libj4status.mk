# Server
noinst_LTLIBRARIES += \
	libj4status.la

libj4status_la_SOURCES = \
	src/libj4status/config.c

libj4status_la_CFLAGS = \
	$(AM_CFLAGS) \
	-D SYSCONFDIR=\"$(sysconfdir)\" \
	-D LIBDIR=\"$(libdir)\" \
	-D DATADIR=\"$(datadir)\" \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS)

libj4status_la_LIBADD = \
	$(GIO_LIBS) \
	$(GLIB_LIBS)
