## cinepaint/lib/fl_i18n/Makefile.am

## Process this file with automake to produce Makefile.in


programincludedir = $(includedir)/cinepaint-$(VERSION)/lib

scriptdata =

EXTRA_DIST = AUTHORS BUGS COPYING ChangeLog INSTALL README TODO threads.h

lib_LTLIBRARIES = libcinepaint_fl_i18n.la 

libcinepaint_fl_i18n_la_SOURCES = \
	fl_i18n_cinepaint.cxx \
	fl_i18n.cxx \
	fl_i18n.H

library_includedir=${pkgincludedir}
#library_include_HEADERS = fl_i18n.H

libcinepaint_fl_i18n_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION):0


INCLUDES = $(USE_THREADS) $(FLTK_CFLAGS) $(X_CFLAGS) \
	-I$(top_srcdir) -I$(includedir)

libcinepaint_fl_i18n_la_LIBADD = \
	$(FLTK_LIBS_PURE) \
	-lc

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

