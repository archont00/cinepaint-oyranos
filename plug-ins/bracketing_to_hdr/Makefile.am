## Process this file with automake to produce Makefile.in

SUBDIRS = br_core FL_adds gui jhead

pluginlibdir = $(programplugindir)/plug-ins

EXTRA_DIST = \
	README \
	AUTHORS \
	NEWS \
	TODO \
	FILES \
	Doxyfile
	

pluginlib_PROGRAMS = bracketing_to_hdr

install-exec-hook:
	fltk-config --post $(DESTDIR)$(pluginlibdir)/bracketing_to_hdr$(EXEEXT)

bracketing_to_hdr_SOURCES = \
	bracketing_to_hdr.hpp \
	bracketing_to_hdr.cpp \
	\
	TNT/tnt_version.h \
	TNT/tnt_i_refvec.hpp \
	TNT/tnt_array1d.hpp \
	TNT/tnt_array1d_utils.hpp \
	TNT/tnt_array2d.hpp \
	TNT/tnt_array2d_utils.hpp \
	TNT/tnt_math_utils.hpp \
	TNT/tnt_misc.hpp \
	TNT/tnt_stopwatch.hpp \
	TNT/tnt_stopwatch_.hpp \
	TNT/jama_qr.hpp \
	TNT/jama_svd.hpp

AM_CXXFLAGS = \
	$(X_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	$(FLTK_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir)

LDADD = \
	$(X_LIBS) \
	$(top_builddir)/lib/libcinepaint.la \
	./br_core/libbr_core.la \
	./FL_adds/libfl_adds.la \
	./gui/libgui.la \
	$(FLTK_LIBS) \
	-lc

DEPS = \
	./br_core/libbr_core.la \
	./FL_adds/libfl_adds.la \
	./gui/libgui.la \
	$(top_builddir)/lib/libcinepaint.la

bracketing_to_hdr_DEPENDENCIES = $(DEPS)

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
