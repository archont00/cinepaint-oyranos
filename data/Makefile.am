## Process this file with automake to produce Makefile.in

SUBDIRS = brushes curves gradients palettes patterns

brushdata=
curvedata=
gradientdata=
palettedata=
patterndata=

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]" | grep -v "CVS"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file | grep -v "CVS"`; \
	  done; \
	done
