## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

pluginlib_PROGRAMS = snoise

snoise_SOURCES = \
	snoise.c   

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = $(top_builddir)/lib/libcinepaint.la	\
	$(X_LIBS)				\
					\
	-lc

DEPS = \
	$(top_builddir)/lib/libcinepaint.la

snoise_DEPENDENCIES = $(DEPS)

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
