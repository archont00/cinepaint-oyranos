## ./Makefile.am

## Process this file with automake to produce Makefile.in

SUBDIRS = libhalf lib libgimp plug-ins app po po-plug-ins po-script-fu data

bin_SCRIPTS = cinepainttool

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cinepaint-gtk.pc

desktopdir = $(DESKTOP_DIR)
desktop_DATA = cinepaint.desktop
pixmapdir = $(PIXMAP_DIR)
pixmap_DATA = cinepaint.png

EXTRA_DIST = \
	gtkrc \
	rmshm.sh \
	tips.txt \
	ps-menurc \
	cinepaint.1 \
	cinepainttool.1 \
	gtkrc.forest2 \
	cinepaint.desktop \
	cinepaint.png \
	cinepaint.m4 \
	cinepaint.spec \
	cinepaint-gtk.pc \
	printrc_user \
	spot.splash.ppm \
	spot.icon.xpm \
	py-compile


##        chain.xpm \
	
programdata_DATA = spot.splash.ppm gimprc gimprc_user gtkrc tips.txt ps-menurc gtkrc.forest2 printrc_user

programdata_SCRIPTS = user_install

man_MANS=cinepaint.1 cinepainttool.1

m4datadir = $(datadir)/aclocal
m4data_DATA = cinepaint.m4

scriptdata =

.PHONY: files populate checkin release

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

populate:
	@echo "populating project"
	@files=`$(MAKE) files | grep -v "make\[[1-9]\]"`; prcs populate -d cinepaint.prj $$files

checkin: populate
	@echo "checking in project"
	@prcs checkin

release:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%y%m%d"`

RPMARCH=`rpmbuild --showrc | awk '/^build arch/ {print $$4}'`

dist-hook:
	mkdir -p $(PACKAGE)-$(VERSION)/data/brushes/big
	for i in @brushdatabig@; do \
	  cp -p $(srcdir)/data/brushes/big/$$i $(PACKAGE)-$(VERSION)/data/brushes/big/; \
	done

rpm: dist
	mkdir -p rpmdir/BUILD \
	rpmdir/SPECS \
	rpmdir/SOURCES \
	rpmdir/SRPMS \
	rpmdir/RPMS/$(RPMARCH)
	cp -f $(PACKAGE)-$(VERSION).tar.gz rpmdir/SOURCES
	rpmbuild --clean -ba $(top_srcdir)/$(PACKAGE).spec --define "_topdir $$PWD/rpmdir"
	@echo "============================================================"
	@echo "Finished - the packages are in rpmdir/RPMS and rpmdir/SRPMS!"

tarball: 
	make distcheck
	make distclean
	sh remove_bak.sh
	sh remove_debug_dir.sh
	tar cvvpzf cinepaint.x.tar.gz cinepaint.x
	gzip -t cinepaint.x.tar.gz 

deb: dist
	@echo "May need: aptitude install git-buildpackage"
	git clone git://git.debian.org/git/collab-maint/cinepaint
	git-buildpackage
