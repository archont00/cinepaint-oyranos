## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

EXTRA_DIST = README INSTALL

pluginlib_PROGRAMS = print

print_SOURCES = \
	icc_common_funcs.c \
	icc_common_funcs.h \
	print-image-gimp.c \
	print-intl.h \
	print-lcms-funcs.h \
	print-lcms-funcs.c \
	print-lcms-options-callbacks.h \
	print-lcms-options.c \
	print-lcms-options.h \
	print.h \
	print.c \
	print_gimp.h \
	xmalloc.c \
	xmalloc.h

INCLUDES = \
	$(X_CFLAGS)			\
 	@PRINT_CFLAGS@			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(X_LIBS)				\
	$(GTK_LIBS)                             \
	@LCMS_LIB@                           \
	@PRINT_LIBS@                           \
	-lc

DEPS = $(top_builddir)/lib/libcinepaint.la

print_DEPENDENCIES = $(DEPS)

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
