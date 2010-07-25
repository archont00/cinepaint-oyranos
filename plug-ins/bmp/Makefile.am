## Process this file with automake to produce Makefile.in

libexecdir = $(programplugindir)/plug-ins

libexec_PROGRAMS = bmp

bmp_SOURCES = \
	bmp.c		\
	bmp.h		\
	bmpread.c	\
	bmpwrite.c   

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
