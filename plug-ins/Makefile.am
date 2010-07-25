## Process this file with automake to produce Makefile.in

##Old RnH plugins	rll		pts			fm_pts		parsley fur

SUBDIRS = \
	blur \
	bmp \
	@BRACKETING@ \
	cineon \
	@COLLECT@ \
	compose \
	dbbrowser \
	decompose \
	dicom \
	edge \
	fits \
	gauss_rle \
	gbr \
	gifload \
	hdr \
	@ICC_EXAMIN@ \
	iff \
	jpeg \
	@IOL@ \
	mblur \
	median \
	minimum \
	noisify \
	openexr \
	@PDF@ \
	pic \
	png \
	pnm \
	@PRINT@ \
	psd \
	psd_save \
	@PYGIMP@ \
	script-fu \
	rawphoto \
	retinex \
	rotate \
	screenshot \
	sgi \
	sharpen \
	snoise \
	sobel \
	spread \
	tga \
	@TIFF@ \
	unsharp \
	xwd
	

##	gif \
## 	gfig \
##	userfilter
##	@OPENEXR@ \


## DIST_SUBDIRS = $(SUBDIRS)

## DISTCLEANFILES = openexr/Makefile openexr/.deps/openexr.Po

scriptdata =

pluginlibdir = $(programplugindir)/plug-ins

AM_CPPFLAGS = \
        -DPRECISION_CONFIG=$(precision)

INCLUDES = \
	$(X_CFLAGS)			\
	-I$(top_srcdir)			\
	-I$(includedir)

LIBCINEPAINTUI = $(top_builddir)/lib/libcinepaintui.la

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
