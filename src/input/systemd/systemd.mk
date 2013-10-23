plugins_LTLIBRARIES += \
	systemd.la \
	$(null)

BUILT_SOURCES += \
	src/input/systemd/dbus.h \
	$(null)

systemd_la_SOURCES = \
	src/input/systemd/dbus.h \
	src/input/systemd/dbus.c \
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
	libj4status.la \
	$(GIO_LIBS) \
	$(GLIB_LIBS) \
	$(null)

XSLTPROC_CONDITIONS += enable_systemd_input

man5_MANS += \
	man/j4status-systemd.conf.5 \
	$(null)

CLEANFILES += \
	src/input/systemd/dbus.h \
	src/input/systemd/dbus.c \
	$(null)

src/input/systemd/dbus.h src/input/systemd/dbus.c: $(dbusinterfacesdir)/org.freedesktop.systemd1.Manager.xml $(dbusinterfacesdir)/org.freedesktop.systemd1.Unit.xml
	$(AM_V_GEN)cd $(builddir)/$(dir $*) && $(GDBUS_CODEGEN) --interface-prefix=org.freedesktop.systemd1 --generate-c-code=$(notdir $(basename $*)) --c-namespace=J4statusSystemd $^
