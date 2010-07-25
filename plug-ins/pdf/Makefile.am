## Process this file with automake to produce Makefile.in

pluginlibexecdir = $(programplugindir)/plug-ins

EXTRA_DIST = \
	pdf_dialog.fl

pluginlibexec_PROGRAMS = pdf

install-exec-hook:
	$(FLTK_CONFIG) --post $(DESTDIR)$(pluginlibexecdir)/$(pluginlibexec_PROGRAMS)$(EXEEXT)

pdf_SOURCES = \
	pdf.cpp \
	pdf_dialog.cxx \
	pdf.h \
	pdf_dialog.h

AM_CXXFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\" \
	$(X_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	$(FLTK_CFLAGS) \
	$(OYRANOS_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la \
	$(X_LIBS) \
	$(FLTK_LIBS) \
	$(OYRANOS_LIBS) \
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

pdf_DEPENDENCIES = $(DEPS)

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
