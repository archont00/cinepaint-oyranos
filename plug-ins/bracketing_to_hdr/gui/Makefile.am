## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

noinst_LTLIBRARIES = libgui.la

libgui_la_LIBADD = \
	../br_core/libbr_core.la \
	../FL_adds/libfl_adds.la \
	$(FLTK_LIBS) 
#        $(top_builddir)/lib/libcinepaint.la \
#        $(GTK_LIBS)

EXTRA_DIST = \
	README \
	gui_rest.cpp \
	DisplcmFinder_UI_rest.cpp \
	gui.fl \
	DisplcmFinder_UI.fl \
	EventTesterWinClass.fl

libgui_la_SOURCES = \
	fl_translate_menu.hpp\
	fl_translate_menu.cpp\
	br_types_plot.hpp\
	br_messages_gui.hpp\
	br_messages_gui.cpp\
	WidgetWrapper.hpp\
	WidgetWrapper.cpp\
	StatusLine.hpp\
	ProgressBar.hpp\
	ProgressBar.cpp\
	TheProgressBar.hpp\
	ImageTable.hpp\
	ImageTable.cpp\
	OffsetTable.hpp\
	OffsetTable.cpp\
	ExifTable.hpp\
	ExifTable.cpp\
	StopvalueChoicer.hpp\
	StopvalueChoicer.cpp\
	RefpicChoicer.hpp\
	RefpicChoicer.cpp\
	EventTester.hpp\
	EventTester.cpp\
	buttons.hpp\
	buttons.cpp\
	FllwUpCrvPlot.hpp\
	FllwUpCrvPlot.cpp\
	ResponsePlot.hpp\
	ResponsePlot.cpp\
	HistogramPlot.hpp\
	HistogramPlot.cpp\
	preferences.hpp\
	preferences.cpp\
	gui.h\
	gui.cxx\
	gui_rest.hpp\
	DisplcmFinder_UI.h\
	DisplcmFinder_UI.cxx\
	EventTesterWinClass.h\
	EventTesterWinClass.cxx\
	AllWindows.hpp\
	AllWindows.cpp
	
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

#libgui_la_DEPENDENCIES = $(DEPS)

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
