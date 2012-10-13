plugins_LTLIBRARIES += \
	systemd.la

BUILT_SOURCES += \
	src/input/systemd/dbus.h

systemd_la_SOURCES = \
	src/input/systemd/dbus.h \
	src/input/systemd/dbus.c \
	src/input/systemd/systemd.c

systemd_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GLIB_CFLAGS)

systemd_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input

systemd_la_LIBADD = \
	libj4status.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS)

XSLTPROC_CONDITIONS += enable_systemd_input

man5_MANS += \
	man/j4status-systemd.conf.5

CLEANFILES += \
	src/input/systemd/dbus.h \
	src/input/systemd/dbus.c

src/input/systemd/dbus.h src/input/systemd/dbus.c: $(dbusinterfacesdir)/org.freedesktop.systemd1.Manager.xml $(dbusinterfacesdir)/org.freedesktop.systemd1.Unit.xml
	$(AM_V_GEN)cd $(builddir)/$(dir $*) && $(GDBUS_CODEGEN) --interface-prefix=org.freedesktop.systemd1 --generate-c-code=$(notdir $(basename $*)) --c-namespace=J4statusSystemd $^
