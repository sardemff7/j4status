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
systemduserunit_DATA =

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
include output/pango/pango.mk
if ENABLE_I3BAR_INPUT_OUTPUT
include input-output/i3bar/i3bar.mk
endif
include input/time/time.mk
include input/file-monitor/file-monitor.mk
include input/systemd/systemd.mk
if ENABLE_NL_INPUT
include input/nl/nl.mk
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

include src/man.mk
include src/units.mk

-include local-rules.mk

#
# Hooks
#

install-data-hook: la-files-install-hook
uninstall-libLTLIBRARIES uninstall-hook: la-files-uninstall-hook

j4_remove_la = $(if $(strip $($(1)_LTLIBRARIES)),( cd $(DESTDIR)$($(1)dir) && rm -f $($(1)_LTLIBRARIES) ))
j4_remove_so_from_la = $(if $(strip $($(1)_LTLIBRARIES)),for la in $($(1)_LTLIBRARIES); do \
		$(INSTALL) "$(abs_builddir)/$(LT_OBJDIR)"/$${la}i "$(DESTDIR)$($(1)dir)"/$${la}; \
	done )

# *.la files cleanup
la-files-install-hook:
	$(call j4_remove_la,lib)
	$(call j4_remove_la,plugins)

# Remove shared object files at uninstall since
# we remove *.la files at install
la-files-uninstall-hook:
	$(call j4_remove_so_from_la,lib)
	$(call j4_remove_so_from_la,plugins)
