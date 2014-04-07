 comma := ,
 empty :=
 space := $(empty) $(empty)

 AM_DISTCHECK_CONFIGURE_FLAGS = \
	--disable-debug

ACLOCAL_AMFLAGS = -I m4 -I src/libnkutils -I src/libgwater ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

bin_PROGRAMS =
noinst_LTLIBRARIES =
lib_LTLIBRARIES =
plugins_LTLIBRARIES =
pkginclude_HEADERS =

check_PROGRAMS =
TESTS =

man1_MANS =
man5_MANS =
dist_aclocal_DATA =
pkgconfig_DATA =

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =

AM_CPPFLAGS += \
	-D LOCALEDIR=\"$(localedir)\" \
	-I $(srcdir)/src/ \
	-I $(builddir)/src/

AM_XSLTPROCFLAGS += \
	--path "$(srcdir)/src/" \
	--path "$(builddir)/src/"

include main/j4status.mk
include libj4status-plugin/libj4status-plugin.mk
include output/flat/flat.mk
include output/debug/debug.mk
if ENABLE_I3BAR_INPUT_OUTPUT
include input-output/i3bar/i3bar.mk
endif
include input/time/time.mk
include input/file-monitor/file-monitor.mk
if ENABLE_NM_INPUT
include input/nm/nm.mk
endif
if ENABLE_UPOWER_INPUT
include input/upower/upower.mk
endif
if ENABLE_SENSORS_INPUT
include input/sensors/sensors.mk
endif
if ENABLE_MPD_INPUT
include input/mpd/mpd.mk
endif
if ENABLE_PULSEAUDIO_INPUT
include input/pulseaudio/pulseaudio.mk
endif
if ENABLE_SYSTEMD_INPUT
include input/systemd/systemd.mk
endif

include src/man.mk

-include local-rules.mk

#
# Hooks
#

install-data-hook: la-files-install-hook
uninstall-hook: la-files-uninstall-hook

# *.la files cleanup
la-files-install-hook:
	cd $(DESTDIR)$(pluginsdir) && \
		rm $(plugins_LTLIBRARIES)

# Remove shared object files at uninstall since
# we remove *.la files at install
la-files-uninstall-hook:
	cd $(DESTDIR)$(pluginsdir) && \
		rm $(plugins_LTLIBRARIES:.la=$(shrext_cmds))
	rmdir $(DESTDIR)$(pluginsdir)
