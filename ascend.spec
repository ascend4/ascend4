Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.8

# Use release 0.* so that other users can do patch releases with a higher number
# and still have the update occur automatically.
Release:	0%{?dist}

Group:		Applications/Engineering
License:	GPLv2+
URL:		http://ascend.cheme.cmu.edu/
Source:		ascend-0.9.8.tar.bz2

Prefix:		%{_prefix}
Packager:	John Pye
Vendor:		Carnegie Mellon University

#------ build dependencies -------
BuildRequires: scons >= 0.96.92
BuildRequires: bison
BuildRequires: flex >= 2.5.4
BuildRequires: swig >= 1.3.24
BuildRequires: gcc-gfortran gcc-c++ >= 4
BuildRequires: blas-devel
BuildRequires: sundials-devel >= 2.4.0
BuildRequires: ipopt-devel >= 3.10
BuildRequires: python-devel >= 2.4
BuildRequires: tk-devel, tk, tcl-devel, tcl, tktable
BuildRequires: graphviz-devel

# ... documentation
# There are no dependencies for documentation as the tarball
# will always contain documentation in compiled form. Only
# when building from subversion are targets formats of the
# documentation files not available.

#------ runtime dependencies --------
Requires: blas%{?_isa}
Requires: sundials%{?_isa}
Requires: ipopt%{?_isa}

# ...pygtk
Requires: python%{?_isa} >= 2.4
Requires: pygtk2 >= 2.6
Requires: pygtk2-libglade
# does this one get picked up automatically?
Requires: python-matplotlib
Requires: numpy
Requires: ipython

# ... file association
Requires(post): desktop-file-utils shared-mime-info
Requires(postun): desktop-file-utils shared-mime-info

# syntax highlighting for gedit
Requires: gtksourceview3

%define pyver %(python -c 'import sys ; print sys.version[:3]')
%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(plat_specific=0)")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(pat_specific=1)")}
%define gtksourceview_lang_file %{_datadir}/gtksourceview-3.0/language-specs/ascend.lang

%{?filter_setup:
%filter_provides_in %{_libdir}/ascend/models/.*\.so$
%filter_provides_in %{_libdir}/ascend/solvers/.*\.so$
%filter_setup
}

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

# for the moment we'll just make one big super-package, to keep things 
# simple for end-users.

%package devel
Summary: Developer files ASCEND
Group: Applications/Engineering
Requires: %{name}
%description devel
Developer files for ASCEND, in the form for C header files for the core
ASCEND library, 'libascend'.

%package doc
Summary: Documentation for ASCEND
Group: Applications/Engineering
%description doc
Documentation for ASCEND, in the form of a PDF User's Manual.

#%package -n libascend1
#Summary: Shared library for core ASCEND functionality
#Group: Applications/Engineering
#%description -n libascend1
#Shared library for ASCEND, providing core functionality including compiler 
#and solver API.

#%package -n ascend-python
#Version:    %{version}
#Summary:    PyGTK user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-python
#PyGTK user interface for ASCEND. This is a new interface that follows GNOME
#human interface guidelines as closely as possible. It does not as yet provide
#access to all of the ASCEND functionality provided by the Tcl/Tk interface.
#

%package tcltk
Summary: Tcl/Tk user interface for ASCEND
Group: Applications/Engineering
Requires: xgraph >= 11
Requires: tcl%{?_isa} >= 8.3
Requires: tk%{?_isa} >= 8.3
Requires: tktable < 2.10, tktable >= 2.8

%description tcltk
Tcl/Tk user interface for ASCEND. This is the original ASCEND IV interface
and is a more complete and mature interface than the alternative PyGTK
interface. Use this interface if you need to use ASCEND *.a4s files or other
functionality not provided by the PyGTK interface.

%prep
%setup -q -n ascend-0.9.8

%build
scons %{_smp_mflags} \
	INSTALL_ROOT=%{buildroot} \
	INSTALL_PREFIX=%{_prefix} \
	INSTALL_SHARE=%{_datadir} \
	INSTALL_BIN=%{_bindir} \
	INSTALL_INCLUDE=%{_includedir} \
	INSTALL_LIB=%{_libdir} \
	INSTALL_DOC=%{_docdir}/%{name}-doc-%{version} \
	DEBUG=1 \
	WITH_DOC_BUILD=0 \
	WITH_DOC_INSTALL=0 \
	WITH_SOLVERS=QRSLV,LSODE,CMSLV,IDA,LRSLV,CONOPT,DOPRI5,IPOPT \
	ABSOLUTE_PATHS=1 \
	%{?__cc:CC="%__cc"} %{?__cxx:CXX="%__cxx"} \
	ascend ascxx pygtk tcltk models solvers

%install
scons %{_smp_mflags} install

# Install menu entry for PyGTK interface, gtksourceview syntax highlighting, and MIME definition
pushd pygtk/gnome
install -m 644 -D ascend.desktop %{buildroot}/%{_datadir}/applications/ascend.desktop
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/ascend-app.png
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend.png
install -m 644 -D ascend.xml %{buildroot}/%{_datadir}/mime/packages/ascend.xml
popd

# file-type icon for ascend models (double click should open in ASCEND)
pushd pygtk/glade
install -m 644 -D ascend-doc-48x48.svg %{buildroot}/%{_datadir}/icons/text-x-ascend-model.svg
popd

# language file for use with gedit
# FIXME gtksourceview-3.0?
pushd tools/gtksourceview-2.0
install -m 644 -D ascend.lang %{buildroot}/%{gtksourceview_lang_file}
popd

# Install menu entry for Tcl/Tk interface
pushd tcltk/gnome
install -m 644 -D ascend4.desktop %{buildroot}/%{_datadir}/applications/ascend4.desktop
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/ascend4-app.png
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend4.png
popd

#/usr/lib/rpm/redhat/brp-strip-shared /usr/bin/strip

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig
update-desktop-database
update-mime-database /usr/share/mime &> /dev/null || :

%postun
/sbin/ldconfig
update-desktop-database
update-mime-database /usr/share/mime &> /dev/null || :

%files
%defattr(644,root,root)
%doc INSTALL.txt LICENSE.txt

%defattr(644,root,root)
%{_libdir}/ascend/models
%{_libdir}/ascend/solvers
%{_datadir}/mime/packages/ascend.xml
%{gtksourceview_lang_file}
%{_datadir}/icons/text-x-ascend-model.svg

#%files -n libascend1
%defattr(755,root,root)
%{_libdir}/libascend.so.*

# %package python
%defattr(755,root,root)
%{_bindir}/ascend
%{python_sitearch}/ascend/_ascpy.so
%defattr(644,root,root)
%{python_sitearch}/ascend/*.py
%{python_sitearch}/ascend/*.py[oc]
%{_datadir}/ascend/glade
%{_datadir}/applications/ascend.desktop
%{_datadir}/icons/ascend-app.png
%{_datadir}/icons/hicolor/64x64/ascend.png

# %package -n python-fprops
%defattr(755,root,root)
%{python_sitearch}/_fprops.so
%defattr(644,root,root)
%{python_sitearch}/fprops.py
%{python_sitearch}/fprops.py[oc]

%files tcltk
%defattr(755,root,root)
%{_bindir}/ascend4
%{_libdir}/libascendtcl.so
%defattr(644,root,root)
%{_datadir}/ascend/tcltk
%{_datadir}/applications/ascend4.desktop
%{_datadir}/icons/ascend4-app.png
%{_datadir}/icons/hicolor/64x64/ascend4.png

%files devel
%defattr(755,root,root)
%{_bindir}/ascend-config
%{_includedir}/ascend
%{_libdir}/lib*.so

%files doc
%defattr(644,root,root)
%doc doc/book.pdf

%changelog
* Wed Dec 12 2012 John Pye <john.pye@anu.edu.au> 0.9.8
- New version

* Thu Apr 30 2009 John Pye <john.pye@anu.edu.au> 0.9.6
- New version

* Wed Jun 25 2008 John Pye <john.pye@anu.edu.au> 0.9.5.115
- New version
- Moved ascend-config to -devel pkg.

* Wed Dec 26 2007 John Pye <john.pye@anu.edu.au> 0.9.5.114
- Minor fixes: error output.
- New 'air properties' model.
- GtkSourceView installed by Scons now.
- Added 'Incidence Graph' feature.

* Sun Aug 19 2007 John Pye <john.pye@anu.edu.au> 0.9.5.113
- External libraries renamed to 'lib<name>_ascend.so' for clarity
  and to solve a Windows-based naming problem.
- Links in Help menu fixed (problem with call to Python webbrowser component).
- License re-tagged according to Fedora requirements.

* Sun Jul 25 2007 John Pye <john.pye@anu.edu.au> 0.9.5.112
- solvers are now all built as separate shared libraries
- mime-type icon added
- RPM now builds on Fedora 5,6,7 and SUSE 10.0 and newer. Not Mandriva though.

* Mon Apr 23 2007 John Pye <john.pye@student.unsw.edu.au> 0.9.5.108
- File ascend.lang has moved.
- book.pdf is included in package.
- some header files have been moved.

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

# vim: set syntax=spec:

