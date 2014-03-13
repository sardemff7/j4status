src/input/mpris/mpris-generated.c:
	$(GDBUS_CODEGEN) --generate-c-code src/input/mpris/mpris-generated src/input/mpris/mpris-interface.xml

CLEANFILES+= \
	src/input/mpris/mpris-generated.c \
	src/input/mpris/mpris-generated.h \
	$(null)

plugins_LTLIBRARIES += \
	mpris.la \
	$(null)

# sources are generated with:
# gdbus-codegen --generate-c-code src/input/mpris/mpris-generated src/input/mpris/mpris-interface.xml
mpris_la_SOURCES = \
	src/input/mpris/mpris-generated.c \
	src/input/mpris/mpris.c \
	src/input/mpris/mpris-generated.h \
	$(null)

mpris_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(null)

mpris_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-module -avoid-version -export-symbols-regex j4status_input \
	$(null)

mpris_la_LIBADD = \
	libj4status-plugin.la \
	libj4status.la \
	$(GIO_LIBS) \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS) \
	$(null)

man5_MANS += \
	man/j4status-mpris.conf.5 \
	$(null)
