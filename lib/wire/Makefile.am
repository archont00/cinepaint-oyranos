## cinepaint/lib/wire/Makefile.am

## Copyright June 1, 2003, Robin.Rowe@MovieEditor.com

## License MIT (http://opensource.org/licenses/mit-license.php)

## Process this file with automake to produce Makefile.in


noinst_LTLIBRARIES = libwire.la



libwire_la_SOURCES = \
	c_typedefs.h \
	dl_list.c \
	dl_list.h \
	datadir.c \
	datadir.h \
	enums.h \
	event.c \
	event.h \
	iodebug.c \
	iodebug.h \
	libtile.c \
	libtile.h \
	precision.h \
	protocol.c \
	protocol.h \
	taskswitch.c \
	taskswitch.h \
	wire.c \
	wire.h \
	wire_types.h \
	wirebuffer.c \
	wirebuffer.h


library_includedir=${pkgincludedir}/wire
library_include_HEADERS = $(wildcard *.h)


AM_CPPFLAGS = -DDATADIR=\""$(programdatadir)"\" -DDOTDIR=\""$(programdotdir)"\" -DPREFIX=\""$(prefix)"\" -DVERSION=\""$(VERSION)"\"




INCLUDES = $(X_CFLAGS) -I$(top_srcdir) -I$(includedir)



## libwire_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION):$(MICRO_VERSION)



.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done


