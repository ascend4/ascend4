# prevent filtering for 'provides' tagging of ASCEND models/solvers
%{?filter_setup:
%filter_provides_in %{_libdir}/ascend/models/.*\.so$
%filter_provides_in %{_libdir}/ascend/solvers/.*\.so$
%filter_setup
}
%{!?python2_sitearch: %global python2_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(pat_specific=1)")}
%global gtksourceview_lang_file %{_datadir}/gtksourceview-3.0/language-specs/ascend.lang

Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.9
Release:	0%{?dist}
License:	GPLv2+
URL:		http://ascend4.org/
Source:		http://ascend4.org/ascend-0.9.9.tar.bz2

#------ build dependencies -------
BuildRequires: scons >= 0.96.92
BuildRequires: bison
BuildRequires: flex >= 2.5.4
BuildRequires: swig >= 1.3.24
BuildRequires: gcc-gfortran >= 4
BuildRequires: blas-devel
BuildRequires: sundials-devel >= 2.4.0
#BuildRequires: tk-devel, tk, tcl-devel, tcl, tktable
BuildRequires: graphviz-devel
BuildRequires: desktop-file-utils
%if 0%{?fedora}
BuildRequires: python2-devel
BuildRequires: coin-or-Ipopt-devel >= 3.10
BuildRequires: MUMPS-devel
BuildRequires: lapack-devel
BuildRequires: CUnit-devel
%else
BuildRequires: python-devel >= 2.4
BuildRequires: gcc-c++ >= 4
BuildRequires: ipopt-devel >= 3.10 or 
%endif

# ... documentation
# There are no dependencies for documentation as the tarball
# will always contain documentation in compiled form. Only
# when building from subversion are targets formats of the
# documentation files not available.

#------ runtime dependencies --------
Requires: blas%{?_isa}
Requires: sundials%{?_isa}
Requires: coin-or-Ipopt%{?_isa}

# ... pygtk
Requires: python%{?_isa} >= 2.4
Requires: pygtk2 >= 2.6
#	^...libglade is no longer required; we use gtk.Builder

Requires: python-matplotlib
Requires: numpy
Requires: ipython
# ... syntax highlighting for gedit
Requires: gtksourceview3

# ... file association
#Requires(post): desktop-file-utils shared-mime-info
#Requires(postun): desktop-file-utils shared-mime-info

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

%package devel
Summary: ASCEND developer files
Requires: %{name} = %{version}-%{release}
%description devel
Developer files for ASCEND, in the form for C header files for the core
ASCEND library, 'libascend'.

%package doc
Summary: ASCEND documentation
Requires: %{name} = %{version}-%{release}
%description doc
Documentation for ASCEND, in the form of a PDF User's Manual.

%prep
%setup -q -n %{name}-%{version}
# note Antonio Trande had some sed actions to insert directories into SConstruct, not sure that's needed actually.

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
	WITH_TCLTK=0 \
	WITH_SOLVERS=QRSLV,LSODE,CMSLV,IDA,LRSLV,CONOPT,DOPRI5,IPOPT \
	ABSOLUTE_PATHS=1 \
	%{?__cc:CC="%{?ccache} %__cc"} %{?__cxx:CXX="%{?ccache} %__cxx"} \
	ascend ascxx pygtk tcltk models solvers

%install
scons install

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
pushd tools/gtksourceview-3.0
install -m 644 -D ascend.lang %{buildroot}/%{gtksourceview_lang_file}
popd

#/usr/lib/rpm/redhat/brp-strip-shared /usr/bin/strip

##Tricks
# Fix .desktop files entries
desktop-file-install \
	--set-icon=ascend-app \
	--remove-key=Encoding \
	%{buildroot}/%{_datadir}/applications/%{name}.desktop

# Fixed execute permission
pushd %{buildroot}/%{_libdir}
 for i in `find . -perm /644 -type f \( -name "*.so" -o -name "*.sh" -o -name "*.py" \)`; do
 chmod a+x $i
done
popd

pushd %{buildroot}%{python2_sitearch}
 for i in `find . -perm /644 -type f \( -name "*.so" -o -name "*.py" \)`; do
 chmod a+x $i
done
popd

chmod a+x %{buildroot}/%{_libdir}/libascend.so.1.0

#for file in %{buildroot}%{johnpye}/fprops/test/{ph,sat,sat1,ideal}; do
#   chmod a+x $file
#done

%post
/sbin/ldconfig
update-desktop-database &> /dev/null || :
update-mime-database /usr/share/mime &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%postun
/sbin/ldconfig
update-desktop-database &> /dev/null || :
update-mime-database /usr/share/mime &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor &>/dev/null
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%defattr(644,root,root)
%doc INSTALL.txt LICENSE.txt

%defattr(644,root,root)
%{_libdir}/ascend/models
%{_libdir}/ascend/solvers
%{_datadir}/mime/packages/ascend.xml
%{gtksourceview_lang_file}
%{_datadir}/icons/text-x-ascend-model.svg

%defattr(755,root,root)
%{_libdir}/libascend.so.*

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

# ...files python-fprops
%defattr(755,root,root)
%{python_sitearch}/_fprops.so
%defattr(644,root,root)
%{python_sitearch}/fprops.py
%{python_sitearch}/fprops.py[oc]

%files devel
%defattr(755,root,root)
%{_bindir}/ascend-config
%{_includedir}/ascend
%{_libdir}/lib*.so

%files doc
%defattr(644,root,root)
%doc doc/book.pdf

%changelog
* Mon Jun 23 2014 John Pye <john.pye@anu.edu.au> 0.9.8
- Incorporating changes from Antonio Trande's official Fedora packaging
- See: http://pkgs.fedoraproject.org/cgit/ascend.git/tree/

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

* Sun Jul 22 2007 John Pye <john.pye@anu.edu.au> 0.9.5.112
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

* Thu Apr 06 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
