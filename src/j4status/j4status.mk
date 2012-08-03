# Server
bin_PROGRAMS += \
	j4status

pkginclude_HEADERS += \
	include/j4status-plugin.h

j4status_SOURCES = \
	src/j4status/plugins.c \
	src/j4status/plugins.h \
	src/j4status/j4status.c

j4status_CFLAGS = \
	$(AM_CFLAGS) \
	-D SYSCONFDIR=\"$(sysconfdir)\" \
	-D LIBDIR=\"$(libdir)\" \
	-D DATADIR=\"$(datadir)\" \
	$(GTHREAD_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GLIB_CFLAGS)

j4status_LDADD = \
	$(GTHREAD_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GMODULE_LIBS) \
	$(GLIB_LIBS)
