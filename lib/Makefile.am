## cinepaint/lib/Makefile.am

## Process this file with automake to produce Makefile.in


SUBDIRS = wire fl_i18n .


programincludedir = $(includedir)/cinepaint-$(VERSION)/lib

scriptdata =

EXTRA_DIST = version.h.in

lib_LTLIBRARIES = libcinepaint.la 

##noinst_LIBRARIES = libcinepaint.a 

libcinepaint_la_LIBADD = \
	./wire/libwire.la \
	$(top_builddir)/libhalf/libcinepaintHalf.la

libcinepaint_la_SOURCES = \
	chain.xpm \
	chainbutton.c	\
	chainbutton.h \
	channel.c \
	channelops_pdb.c \
	channelops_pdb.h \
	compat.h \
	config.h \
	convert_pdb.c \
	convert_pdb.h \
	dialog.c \
	dialog.h \
	display.c \
	dll_api.h \
	drawable.c \
	edit_pdb.c \
	edit_pdb.h \
	export.c \
	export.h \
	fileops_pdb.h \
	fileops_pdb.c \
	float16.h \
	gradient.c \
	guides_pdb.c \
	guides_pdb.h \
	helpui.c \
	helpui.h \
	image.c \
	image_convert.c \
	image_convert.h \
	image_limits.h \
	image_pdb.c \
	image_pdb.h \
	intl.h \
	matrix.c \
	matrix.h \
	menu.c \
	menu.h \
	palette.c \
	pdb.h \
	pixelrgn.c \
	pixmap.c \
	pixmap.h \
	player.c \
	plugin_main.c \
	plugin_main.h \
	plugin_pdb.c \
	plugin_pdb.h \
	rint.h \
	selection_pdb.c \
	selection_pdb.h \
	size_entry.c \
	size_entry.h \
	ui.h \
	undo_pdb.c \
	undo_pdb.h \
	unit.c \
	unit.h \
	unit_pdb.c \
	unit_pdb.h \
	unitmenu.c \
	unitmenu.h \
	version.h \
	widgets.c \
	widgets.h

library_includedir=${pkgincludedir}
library_include_HEADERS = $(wildcard *.h)

## libcinepaint_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION):$(MICRO_VERSION)

libcinepaint_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION):0 -lm -lX11


INCLUDES = $(X_CFLAGS) \
	-I$(top_srcdir)	-I$(includedir)	\
	-I../wire -I../fl_i18n

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

