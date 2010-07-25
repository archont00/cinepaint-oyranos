## Process this file with automake to produce Makefile.in

##if HAVE_OPENEXR

pluginlibdir = $(programplugindir)/plug-ins

pluginlib_PROGRAMS = openexr

openexr_SOURCES = \
	chroma_icc.cxx \
	chroma_icc.h \
	openexr.cxx
##endif

AM_CXXFLAGS = \
	$(X_CXXFLAGS) \
	$(OPENEXR_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir)/OpenEXR \
	-I$(includedir) \
        @OPENEXR_INCLUDES@ 

LDADD = \
	$(top_builddir)/lib/libcinepaint.la \
	$(X_LIBS) \
	$(OPENEXR_LIBS) \
	@LCMS_LIB@ \
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

## MOSTLYCLEANFILES = .deps/openexr.Po

openexr_DEPENDENCIES = $(DEPS)

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



