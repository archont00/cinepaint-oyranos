AUTOMAKE_OPTIONS = 1.4a

SUBDIRS = doc plug-ins

INCLUDES = $(PYTHON_INCLUDES) \
	@PYINCLUDE@ \
	$(PYTHON_CFLAGS) \
	-I../.. \
	$(GTK_CFLAGS) \
	$(GIMP_CFLAGS_NOUI)  

pyexec_LTLIBRARIES = gimpmodule.la

gimpmodule_la_SOURCES = gimpmodule.c

gimpmodule_la_LIBADD = \
	$(top_builddir)/lib/libcinepaint.la \
	$(X_LIBS) \
	-lc \
	$(GIMP_LIBS_NOUI) \
	@PYLIB@ @PYLINK@

gimpmodule_la_LDFLAGS = -module -avoid-version

pysitedir = $(PYTHON_SITE)
pygimp_PYTHON = gimpplugin.py gimpenums.py gimpshelf.py \
	gimpui.py gimpfu.py

#pygimpbase = python

pygimpdir = $(pythondir)
            #$(programplugindir)/$(pygimpbase)

EXTRA_DIST = pygimp.spec


#pyenvdir = $(programplugindir)/environ
#pyinterpdir = $(programplugindir)/interpreters

#pyenvfile = $(pyenvdir)/pygimp.env
#pyinterpfile = $(pyinterpdir)/pygimp.interp

#install-env-file:
#	$(mkinstalldirs) '$(DESTDIR)$(pyenvdir)'
#	echo '$(path_separator) PYTHONPATH=$${gimp_plug_in_dir}/$(pygimpbase)' > '$(DESTDIR)$(pyenvfile)'

#install-interp-file:
#	$(mkinstalldirs) '$(DESTDIR)$(pyinterpdir)'
#	echo 'python=$(PYBIN_PATH)' > '$(DESTDIR)$(pyinterpfile)'
#	echo '/usr/bin/python=$(PYBIN_PATH)' >> '$(DESTDIR)$(pyinterpfile)'
#	echo ':Python:E::py::python:' >> '$(DESTDIR)$(pyinterpfile)'

#install-data-local: install-env-file install-interp-file

#uninstall-local:
#	rm -f '$(DESTDIR)$(pyenvfile)' '$(DESTDIR)$(pyinterpfile)'



snap:
	$(MAKE) dist distdir=$(PACKAGE)-SNAP-`date +"%Y%m%d"`

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
