## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

pluginlib_PROGRAMS = png

png_SOURCES = \
	png.c   

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir) \
	$(PNG_INCLUDES)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(LIBPNG)				\
	$(GTK_LIBS)				\
	$(X_LIBS)				\
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

png_DEPENDENCIES = $(DEPS)

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
