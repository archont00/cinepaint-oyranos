## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins
extralibdir = $(programplugindir)/extra

pluginlib_PROGRAMS = \
	rawphoto

extralib_PROGRAMS = \
	dcraw

rawphoto_SOURCES = \
	rawphoto.c

dcraw_SOURCES = \
	dcraw.c

INCLUDES = \
	$(X_CFLAGS)			\
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(X_LIBS)				\
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

rawphoto_DEPENDENCIES = $(DEPS)

dcraw_LDADD = \
	$(LCMS_LIB) \
	-ljpeg -lz -lc -lm

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

