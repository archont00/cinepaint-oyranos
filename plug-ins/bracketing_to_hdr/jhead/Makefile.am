## Process this file with automake to produce Makefile.in

extralibdir = $(programplugindir)/extra

EXTRA_DIST = \
	changes.txt \
	jhead.1.gz \
	make.bat \
	makefile-unix \
	makefile-win32 \
	myglob.c \
	readme.txt \
	usage.html \
	README

extralib_PROGRAMS = \
	jhead

jhead_SOURCES = \
	jhead.c \
	jhead.h \
	jpgfile.c \
	exif.c \
	gpsinfo.c \
	makernote.c

INCLUDES = \
	$(X_CFLAGS)			\
	-DLOCALEDIR=\""$(localedir)"\" \
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	-lc -lm

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

