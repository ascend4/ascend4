Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.5.108

# Use release 0.* so that other users can do patch releases with a higher number
# and still have the update occur automatically.
Release:	0

License:	GPL
Group:		Applications/Engineering
Source:		ascend-0.9.5.108.tar.bz2
URL:		http://ascend.cheme.cmu.edu/

Prefix:		%{_prefix}
Packager:	John Pye
Vendor:		Carnegie Mellon University
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

BuildRequires: python >= 2.4, python-devel
BuildRequires: scons >= 0.96.91
BuildRequires: bison >= 2.0
BuildRequires: flex >= 2.5.4
BuildRequires: swig >= 1.3.24
BuildRequires: tk-devel >= 8.3, tcl-devel >= 8.3
BuildRequires: tktable < 2.10, tktable >= 2.8
BuildRequires: desktop-file-utils
BuildRequires: sundials >= 2.2.1
BuildRequires: conopt >= 3.14

# This contains the libg2c library; which on FC5 is not in the path, unfort.
BuildRequires: compat-gcc-32-g77 == 3.2.3

Requires: python >= 2.4
Requires: pygtk2 >= 2.6
Requires: pygtk2-libglade
Requires: python-matplotlib
Requires: python-numeric
Requires: gtksourceview
Requires: xgraph >= 11
# sundials is statically linked (hopefully)

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

#%package -n ascend-python
#Version:    0.9.5.108
#Summary:    PyGTK user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-python
#PyGTK user interface for ASCEND. This is a new interface that follows GNOME
#human interface guidelines as closely as possible. It does not as yet provide
#access to all of the ASCEND functionality provided by the Tcl/Tk interface.
#
#%package -n ascend-tcltk
#Version:    0.9.5.108
#Summary:    Tcl/Tk user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-tcltk
#Tcl/Tk user interface for ASCEND. This is the original ASCEND IV interface
#and is a more complete and mature interface than the alternative PyGTK
#interface. Use this interface if you need to use ASCEND *.a4s files or other
#functionality not provided by the PyGTK interface.

%prep
%setup -q -n ascend-0.9.5.108

%build
scons %{_smp_mflags} DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models \
	INSTALL_ROOT=%{buildroot} \
	INSTALL_PREFIX=%{_prefix} \
	INSTALL_SHARE=%{_datadir} \
	INSTALL_BIN=%{_bindir} \
	INSTALL_INCLUDE=%{_includedir} \
	F2C_LIBPATH=/usr/lib/gcc-lib/i386-redhat-linux/3.2.3/ \
	WITH_SOLVERS=QRSLV,LSOD,CMSLV,LRSLV,CONOPT \
	pygtk tcltk

%install
rm -rf %{buildroot}
scons %{_smp_mflags} install

# Install menu entry for PyGTK interface, gtksourceview syntax highlighting, and MIME definition
pushd pygtk/gnome
install -m 644 -D ascend.desktop %{buildroot}/%{_datadir}/applications/ascend.desktop
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/ascend-app.png
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend.png
install -m 644 -D ascend.xml %{buildroot}/%{_datadir}/mime/packages/ascend.xml
popd
pushd tools/gedit
install -m 644 -D ascend.lang %{buildroot}/%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
popd

# Install menu entry for Tcl/Tk interface
pushd tcltk/gnome
install -m 644 -D ascend4.desktop %{buildroot}/%{_datadir}/applications/ascend4.desktop
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/ascend4-app.png
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend4.png
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
%doc INSTALL.txt LICENSE.txt
%{_bindir}/ascend-config
%{_datadir}/ascend/models
%{_libdir}/libascend.so
%{_datadir}/mime/packages/ascend.xml
%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang

# %package -n ascend-python
%{_bindir}/ascend
%{_datadir}/ascend/*.py
%{_datadir}/ascend/*.pyc
%{_datadir}/ascend/glade
%{_datadir}/ascend/_ascpy.so
%{_datadir}/applications/ascend.desktop
%{_datadir}/icons/ascend-app.png
%{_datadir}/icons/hicolor/64x64/ascend.png

# %package -n ascend-tcltk
%{_bindir}/ascend4
%{_datadir}/ascend/tcltk
%{_libdir}/libascendtcl.so
%{_datadir}/applications/ascend4.desktop
%{_datadir}/icons/ascend4-app.png
%{_datadir}/icons/hicolor/64x64/ascend4.png

# %package -b ascend-devel
%{_includedir}/compiler
%{_includedir}/general
%{_includedir}/utilities
%{_includedir}/solver

%changelog
* Mon Apr 23 2007 John Pye <john.pye@student.unsw.edu.au>
- File ascend.lang has moved

* Fri Jul 28 2006 John Pye <john.pye@student.unsw.edu.au>
- Added CONOPT support

* Wed Jul 12 2006 John Pye <john.pye@student.unsw.edu.au>
- Fixed fortran linking
- Removed ccache dependency
- Added xgraph dependency
- Added 'include' files plus 'ascend-config' script

* Thu Jun 01 2006 John Pye <john.pye@student.unsw.edu.au>
- Add Tcl/Tk interface to GNOME menu

* Tue May 02 2006 John Pye <john.pye@student.unsw.edu.au>
- Break out ascend-core, ascend-python and ascend-tcltk packages.

* Mon Apr 24 2006 John Pye <john.pye@student.unsw.edu.au>
- Modified for removed dir in pygtk source hierachy

* Thu Apr 04 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
