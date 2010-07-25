## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

noinst_LTLIBRARIES = libfl_adds.la

libfl_adds_la_LIBADD = \
	$(FLTK_LIBS) 
#	../br_core/libbr_core.la \
#        $(top_builddir)/lib/libcinepaint.la \
#        $(GTK_LIBS)

EXTRA_DIST = \
	README

libfl_adds_la_SOURCES = \
	Fl_Table.H\
	Fl_Table.cxx\
	Fl_Table_Row.H\
	Fl_Table_Row.cxx\
	fl_eventnames.hpp\
	fl_eventnames.cpp\
	fl_print_event.hpp\
	fl_print_event.cpp\
	fl_misc.hpp\
	fl_misc.cpp\
	Fl_Cartesius.hpp\
	Fl_Cartesius.cpp\
	CurveTnt.hpp\
	Fl_IntInput_Slider.hpp\
	Fl_IntInput_Slider.cpp\
	IntInput_Slider.hpp\
	IntInput_Slider.cpp\
	ys_dbg.hpp\
	ys_dbg_varia.hpp\
	ys_dbg.cpp

AM_CXXFLAGS = \
	$(X_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	$(FLTK_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir) \
        -I..

#LDADD = \
#	$(top_builddir)/lib/libcinepaint.la \
#	$(X_LIBS) \
#	$(FLTK_LIBS) \
#	-lc

#DEPS = $(top_builddir)/lib/libcinepaint.la

#libfl_adds_la_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
