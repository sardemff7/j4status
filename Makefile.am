AM_DISTCHECK_CONFIGURE_FLAGS = \
	--disable-debug

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

pluginsdir = $(pkglibdir)/plugins

bin_PROGRAMS =
noinst_LTLIBRARIES =
plugins_LTLIBRARIES =
pkginclude_HEADERS =
man1_MANS =
man5_MANS =

EXTRA_DIST =
CLEANFILES =

AM_CFLAGS += \
	-D LOCALEDIR=\"$(localedir)\" \
	--include config.h \
	-I $(srcdir)/include/ \
	-I $(builddir)/include/

AM_XSLTPROCFLAGS += \
	--path "$(srcdir)/man/" \
	--path "$(builddir)/man/"

include src/j4status/j4status.mk
include src/libj4status/libj4status.mk
include src/output/flat/flat.mk
if ENABLE_I3BAR_OUTPUT
include src/output/i3bar/i3bar.mk
endif
include src/input/time/time.mk
include src/input/file-monitor/file-monitor.mk
if ENABLE_NM_INPUT
include src/input/nm/nm.mk
endif
if ENABLE_UPOWER_INPUT
include src/input/upower/upower.mk
endif


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
