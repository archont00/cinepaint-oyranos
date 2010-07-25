## Process this file with automake to produce Makefile.in

SUBDIRS = scripts

scriptdata =

pluginlibdir = $(programplugindir)/plug-ins

pluginlib_PROGRAMS = script-fu

AM_CPPFLAGS = -DREGEX_MALLOC

script_fu_SOURCES = \
	gimpcolorbutton.h	\
	gimpcolorbutton.c	\
	gimpfileselection.c	\
	gimpfileselection.h	\
	gimpgradientmenu.h	\
	gimpgradientmenu.c	\
	gimpgradientselect.h	\
	gimpgradientselect.c	\
	gimpgradientselect_pdb.h	\
	gimpgradientselect_pdb.c	\
	gimpmenu.h		\
	gimpmenu.c		\
	gtk1_compat.h		\
	interp_regex.c		\
	interp_slib.c		\
	interp_sliba.c		\
	interp_trace.c		\
	regex.c			\
	regex.h			\
	script-fu.c		\
	script-fu-console.c	\
	script-fu-console.h	\
	script-fu-constants.c	\
	script-fu-constants.h	\
	script-fu-intl.h	\
	script-fu-scripts.c	\
	script-fu-scripts.h	\
	script-fu-server.c	\
	script-fu-server.h	\
	script-fu-enums.h	\
	siod.h			\
	siodp.h

EXTRA_DIST = \
	pixmaps/yes.xpm	\
	pixmaps/no.xpm	\
	script-fu.ico		\
	script-fu.rc

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o	\
	$(top_builddir)/lib/libcinepaint.la			\
	$(X_LIBS)						\
	-lc

DEPS = \
	$(top_builddir)/plug-ins/dbbrowser/dbbrowser_utils.o	\
	$(top_builddir)/lib/libcinepaint.la

script_fu_DEPENDENCIES = $(DEPS)

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
