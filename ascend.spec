
Name:		ascend
Summary:	ASCEND Modelling Environment
Version:	0.9.6rc0

# Use release "0" so that distro-released versions will override ours.
Release:	0.jdpipe

License:	GPL
Group:		Applications/Graphics
Source:		%{name}-%{version}.tar.gz
URL:		http://inkscape.sourceforge.net/

Prefix:		%{_prefix}
Packager:	Automatic
Vendor:		The Inkscape Project
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

BuildRequires:  python-devel
BuildRequires:  desktop-file-utils
Requires(post):   desktop-file-utils
Requires(postun): desktop-file-utils

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

%prep
%setup

%build
scons %{?_smp_mflags}

%install
rm -rf %{buildroot}
scons INSTALL_PREFIX=%{_prefix} INSTALL_DATA=%{_datadir} INSTALL_BIN=%{_bindir} install

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README doc/keys.html
%{_bindir}/ascend
%{_datadir}/applications/ascend.desktop
%{_datadir}/ascend/*

%changelog
* Thu Apr 04 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
