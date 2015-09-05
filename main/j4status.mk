# Server
bin_PROGRAMS += \
	j4status \
	$(null)

systemduserunit_DATA += \
	main/units/j4status@.service \
	main/units/j4status@.socket \
	$(null)

j4status_SOURCES = \
	libj4status-plugin/include/j4status-plugin-private.h \
	main/src/plugins.c \
	main/src/plugins.h \
	main/src/io.c \
	main/src/io.h \
	main/src/j4status.c \
	main/src/j4status.h \
	main/src/types.h \
	$(null)

j4status_CFLAGS = \
	$(AM_CFLAGS) \
	-D SYSCONFDIR=\"$(sysconfdir)\" \
	-D LIBDIR=\"$(libdir)\" \
	-D DATADIR=\"$(datadir)\" \
	$(SYSTEMD_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

j4status_LDADD = \
	libj4status-plugin.la \
	$(SYSTEMD_LIBS) \
	$(GTHREAD_LIBS) \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GMODULE_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man1_MANS += \
	main/man/j4status.1 \
	$(null)

man5_MANS += \
	main/man/j4status.conf.5 \
	$(null)
