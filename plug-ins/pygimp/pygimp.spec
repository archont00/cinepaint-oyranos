%define ver 1.2
%define prefix /usr

%define py_ver 1.5
%define gimp_ver 1.2

Name: pygimp
Version: %ver
Release: 1
Summary: A python extension allowing you to write Gimp plugins in Python
Copyright: GPL
Group: Applications/Graphics
Packager: James Henstridge <james@daa.com.au>
Requires: gimp >= 1.2, python, pygtk

Source: ftp://ftp.daa.com.au/pub/james/pygimp/pygimp-%{ver}.tar.gz
BuildRoot: /tmp/pygimp-root

%description
pygimp allows you to write Gimp plugins with the python language.  Unlike
script-fu scripts which only have access to functions in the PDB (procedural
database), pygimp plugins have access to all functionality that C plugins
have, including direct pixel manipulation that is required for many plugins.

%prep
%setup
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix}

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%files
%defattr(-, root, root)

%{prefix}/lib/python%{py_ver}/site-packages/gimpmodule.*
%{prefix}/lib/python%{py_ver}/site-packages/gimpenums.py*
%{prefix}/lib/python%{py_ver}/site-packages/gimpfu.py*
%{prefix}/lib/python%{py_ver}/site-packages/gimpplugin.py*
%{prefix}/lib/python%{py_ver}/site-packages/gimpshelf.py*
%{prefix}/lib/python%{py_ver}/site-packages/gimpui.py*

%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/clothify.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/foggify.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/gimpcons.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/gtkcons.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/pdbbrowse.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/shadow_bevel.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/sphere.py
%{prefix}/lib/gimp/%{gimp_ver}/plug-ins/whirlpinch.py

%doc README NEWS COPYING doc/*.html