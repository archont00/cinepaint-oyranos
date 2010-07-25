## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

pluginlib_PROGRAMS = jpeg

jpeg_SOURCES = \
	iccjpeg.h \
	iccjpeg.c \
	jpeg.c

INCLUDES = \
	$(X_CFLAGS)			\
	$(OYRANOS_CFLAGS) \
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(X_LIBS)				\
	$(OYRANOS_LIBS) \
	@LCMS_LIB@				\
	@LIBJPEG_LIB@				\
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

jpeg_DEPENDENCIES = $(DEPS)

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
