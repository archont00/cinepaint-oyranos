Summary: CinePaint is a tool for manipulating images.
Name: cinepaint
Version: 0.25
Release: 1
License: GPL,LGPL,MIT
Group: Applications/Multimedia
Source0: http://prdownloads.sourceforge.net/cinepaint/cinepaint-0.25.1.tar.gz
URL: http://www.cinepaint.org
BuildRoot: %{_tmppath}/%{name}-root
#Requires: /sbin/ldconfig FIXME: Find dependencies
Prefix:    %{_prefix}

%description
CinePaint is a painting and retouching tool primarily used for motion
picture frame-by-frame retouching and dust-busting. It was used on THE
LAST SAMURAI, HARRY POTTER and many other films.

CinePaint runs on all popular flavors of Linux and on Mac OS X as an
X11 application. The Windows port of CinePaint is currently broken,
sorry.

CinePaint is different from other painting tools because it supports
deep color depth image formats up to 32 bits per channel deep and ICC
style colour management. 

CinePaint was originally based on GIMP and consequently is a GTK1-based
application. A new FLTK-based version of CinePaint, called Glasgow,
is nearing alpha. There's also a new image core in development, called
img_img, That will enable CinePaint to operate on images from the
command-line and to integrate with other projects such as Blender.

Support from the film industry launched development in 1998. Motion
picture technology company Silicon Grail (later acquired by Apple) and
motion picture studio Rhythm & Hues led the development, with a goal
of creating a deep paint alternative to the recently discontined SGI
IRIX version of Adobe Photoshop and to support the emerging Linux
platform. Although continuously in use in the film industry, it
never had much awareness in the open source community. On July 4,
2002, Robin Rowe released CinePaint as a SourceForge project.

%package devel
Group: Development/Multimedia
Summary: CinePaint is a tool for manipulating images.
%description devel
Developer files for external cinepaint plug-ins.


%prep
%setup -q -n %{name}-%{version}.%{release}

%build
%configure --disable-icc_examin
make %{_smp_mflags}

%install
rm -rf %{buildroot}
make install \
        DESTDIR=%{buildroot}
#mkdir -p %{buildroot}/usr/include/cinepaint-%{version}-%{release}/lib
#mkdir -p %{buildroot}/usr/include/cinepaint-%{version}-%{release}/libgimp
#cp $RPM_BUILD_ROOT/cinepaint-%{version}-%{release}/lib/*.h %{buildroot}/usr/include/cinepaint-%{version}-%{release}/lib/
#cp $RPM_BUILD_ROOT/cinepaint-%{version}-%{release}/libgimp/*.h %{buildroot}/usr/include/cinepaint-%{version}-%{release}/libgimp/
#install -d $RPM_BUILD_ROOT/%{prefix}/include/cinepaint-%{version}-%{release}/lib
#install -d $RPM_BUILD_ROOT/%{prefix}/include/cinepaint-%{version}-%{release}/libgimp
#install -m 644 %{prefix}/src/redhat/BUILD/cinepaint-%{version}-%{release}/lib/*.h $RPM_BUILD_ROOT/%{prefix}/include/cinepaint-%{version}-%{release}/lib
#install -m 644 %{prefix}/src/redhat/BUILD/cinepaint-%{version}-%{release}/libgimp/*.h $RPM_BUILD_ROOT/%{prefix}/include/cinepaint-%{version}-%{release}/libgimp


%clean
rm -rf %{buildroot}

%post

%postun

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog README
%{_bindir}/*
%{_libdir}/*.so.*
%{_prefix}/share/aclocal/cinepaint.m4
%{_mandir}/man1/*
%{_libdir}/cinepaint/0.25.1/*
%{_datadir}/applications/cinepaint.desktop
%{_datadir}/pixmaps/cinepaint.png
#%{_datadir}/fonts/FreeSans.ttf
%{_datadir}/%{name}/%{version}.%{release}/gimprc
%{_datadir}/%{name}/%{version}.%{release}/gimprc_user
%{_datadir}/%{name}/%{version}.%{release}/gtkrc
%{_datadir}/%{name}/%{version}.%{release}/gtkrc.forest2
%{_datadir}/%{name}/%{version}.%{release}/printrc_user
%{_datadir}/%{name}/%{version}.%{release}/ps-menurc
%{_datadir}/%{name}/%{version}.%{release}/spot.splash.ppm
%{_datadir}/%{name}/%{version}.%{release}/tips.txt
%{_datadir}/%{name}/%{version}.%{release}/user_install
%{_datadir}/locale/*
%{_datadir}/%{name}/%{version}.%{release}/brushes/
%{_datadir}/%{name}/%{version}.%{release}/curves/
%{_datadir}/%{name}/%{version}.%{release}/gradients/
%{_datadir}/%{name}/%{version}.%{release}/iol/
%{_datadir}/%{name}/%{version}.%{release}/palettes/
%{_datadir}/%{name}/%{version}.%{release}/patterns/
%{_datadir}/%{name}/%{version}.%{release}/scripts/
%{_prefix}/%{_lib}/python2.6/site-packages

%files devel
%defattr(-, root, root)
#%doc HACKING
%{_libdir}/*.a
%{_libdir}/*.la
%{_libdir}/*.so
%{_libdir}/pkgconfig/cinepaint-gtk.pc
%{_prefix}/include/cinepaint/*
#%{_mandir}/man?/*


%changelog

* Mon May 29 2006 Kai-Uwe Behrmann
- move desktop entries to normal installation

* Mon Mar 26 2006 Kai-Uwe Behrmann
- add locale info

* Fri Apr 11 2003 Robin Rowe
- filmgimp to cinepaint rename
- rename gimp files

* Tue Dec 03 2002 Sam Richards
- Changed include directory path.

* Wed Nov 06 2002 Rene Rask
- release 0.6-2
- Rpms can now be built from source by running "make rpm"
- Updated splash and logo images.

* Mon Nov 04 2002 Rene Rask
- release 0.6.

* Wed Oct 30 2002 Rene Rask
- Initial RPM release.
