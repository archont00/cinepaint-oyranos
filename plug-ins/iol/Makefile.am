## Process this file with automake to produce Makefile.in

SUBDIRS = examples

pluginlibdir = $(programplugindir)/plug-ins

EXTRA_DIST = \
	AUTHORS COPYING README \
	bytecode.txt iol.txt \
	iolgui.glade \
	lexer.l parser.y

pluginlib_PROGRAMS = iol

iol_SOURCES = \
	callbacks.h \
	callbacks.c \
	fg.h \
	interface.h \
	interface.c \
	iol.h \
	lex.yy.c \
	main.c \
	parser.tab.h \
	parser.tab.c \
	support.h \
	support.c


INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/lib/libcinepaint.la	\
	$(X_LIBS)				\
	-lc					\
	-lm					\
	-lfl

DEPS = $(top_builddir)/lib/libcinepaint.la

iol_DEPENDENCIES = $(DEPS)

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
