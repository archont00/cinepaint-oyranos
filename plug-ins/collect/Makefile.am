## Process this file with automake to produce Makefile.in

pluginlibexecdir = $(programplugindir)/plug-ins

pluginlibexec_PROGRAMS = collect

install-exec-hook:
	$(FLTK_CONFIG) --post $(DESTDIR)$(pluginlibexecdir)/$(pluginlibexec_PROGRAMS)$(EXEEXT)

collect_SOURCES = \
	collect.cpp

AM_CXXFLAGS = \
	$(X_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	$(FLTK_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la \
	$(X_LIBS) \
	$(FLTK_LIBS) \
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la \
	$(top_builddir)/lib/fl_i18n/libcinepaint_fl_i18n.la

collect_DEPENDENCIES = $(DEPS)

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
