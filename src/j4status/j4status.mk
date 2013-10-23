# Server
bin_PROGRAMS += \
	j4status \
	$(null)

j4status_SOURCES = \
	include/j4status-plugin-private.h \
	src/j4status/plugins.c \
	src/j4status/plugins.h \
	src/j4status/j4status.c \
	$(null)

j4status_CFLAGS = \
	$(AM_CFLAGS) \
	-D SYSCONFDIR=\"$(sysconfdir)\" \
	-D LIBDIR=\"$(libdir)\" \
	-D DATADIR=\"$(datadir)\" \
	$(GTHREAD_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

j4status_LDADD = \
	libj4status.la \
	$(GTHREAD_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GMODULE_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man1_MANS += \
	man/j4status.1 \
	$(null)

man5_MANS += \
	man/j4status.conf.5 \
	$(null)
