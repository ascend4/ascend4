
Name:		ascend
Summary:	ASCEND Modelling Environment
Version:	0.9-svn.601

# Use release "0" so that distro-released versions will override ours.
Release:	0.jdpipe

License:	GPL
Group:		Applications/Graphics
Source:		%{name}-%{version}.tar.bz2
URL:		http://inkscape.sourceforge.net/

Prefix:		%{_prefix}
Packager:	John Pye
Vendor:		Carnegie Mellon University
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

BuildRequires: python >= 2.4, python-devel
BuildRequires: scons >= 0.96.1
BuildRequires: bison, flex
BuildRequires: swig >= 1.3.24
BuildRequires: desktop-file-utils

Requires(post):   desktop-file-utils
Requires(postun): desktop-file-utils

Requires: python >= 2.4
Requires: pygtk2 >= 2.6, pygtk2-libglade
Requires: python-matplotlib, python-numeric
Requires: gtksourceview
Requires: make
Requires: gcc

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

%prep
%setup -n ascend -q

%build
scons %{?_smp_mflags} DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models

%install
rm -rf %{buildroot}
scons DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models INSTALL_ROOT=%{buildroot} INSTALL_PREFIX=%{_prefix} INSTALL_DATA=%{_datadir} INSTALL_BIN=%{_bindir} INSTALL_INCLUDE=%{_incdir} install

pushd pygtk/gnome
install -o root -g root -m 644 -D ascend.desktop %{buildroot}/%{_datadir}/applications/ascend.desktop
install -o root -g root -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/ascend-app.png
install -o root -g root -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend.png
install -o root -g root -m 644 -D ascend.xml %{buildroot}/%{_datadir}/mime/packages/ascend.xml
install -o root -g root -m 644 -D ascend.lang %{buildroot}/%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
popd

%clean
rm -rf %{buildroot}

%post
update-desktop-database
update-mime-database /usr/share/mime

%postun
update-desktop-database
update-mime-database /usr/share/mime

%files
%defattr(-, root, root)
%doc INSTALL tcltk98/release_notes/license.txt
%{_bindir}/ascend
%{_datadir}/applications/ascend.desktop
%{_datadir}/ascend/*
%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
%{_datadir}/icons/ascend-app.png
%{_datadir}/icons/hicolor/64x64/ascend.png
%{_datadir}/mime/packages/ascend.xml


%changelog
* Mon Apr 24 2006 John Pye <john.pye@student.unsw.edu.au>
- Modified for removed dir in pygtk source hierachy

* Thu Apr 04 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
