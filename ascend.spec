%define pyver %(python -c 'import sys ; print sys.version[:3]')

%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")}
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(1))")}

%define gtksourceview_lang_file %{_datadir}/gtksourceview-3.0/language-specs/ascend.lang

Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.8
Release:	0.1.4424svn%{?dist}
Group:		Applications/Engineering
License:	GPLv2+
URL:		http://ascend.cheme.cmu.edu/
# This package has been obtained by executing:
# svn co svn://ascend4.org/code/trunk ascend (changeset 4424)
# tar -cvzf  ascend-0.9.8.tar.gz ascend
Source0:	ascend-0.9.8.tar.gz

Buildroot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

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
#BuildRequires: lyx 
#BuildRequires: texlive-collection-fontsrecommended 
#BuildRequires: texlive-epstopdf 
#BuildRequires: texlive-ulem 
#BuildRequires: texlive-lm-math
BuildRequires: desktop-file-utils

#------ runtime dependencies --------
Requires: blas%{?_isa}
Requires: sundials%{?_isa}
Requires: ipopt%{?_isa}

# ...pygtk
Requires: python%{?_isa} >= 2.4
Requires: pygtk2 >= 2.6
# fairly sure we don't need this any more? JP -- using GtkBuilder now, part of core PyGTK
#Requires: pygtk2-libglade
Requires: python-matplotlib
Requires: numpy
Requires: ipython

# ... file association
Requires(post): desktop-file-utils shared-mime-info
Requires(postun): desktop-file-utils shared-mime-info

# syntax highlighting for gedit
Requires: gtksourceview3

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
%setup -q -n %{name}

%build
scons %{_smp_mflags} \
	INSTALL_ROOT=%{buildroot} \
	INSTALL_PREFIX=%{_prefix} \
	INSTALL_SHARE=%{_datadir} \
	INSTALL_BIN=%{_bindir} \
	INSTALL_INCLUDE=%{_includedir} \
	INSTALL_LIB=%{_libdir} \
	INSTALL_DOC=%{_docdir}/%{name}-%{version} \
	DEBUG=1 \
	WITH_DOC_BUILD=1 \
	WITH_DOC_INSTALL=1 \
	WITH_SOLVERS=QRSLV,LSODE,CMSLV,IDA,LRSLV,CONOPT,DOPRI5,IPOPT \
	ABSOLUTE_PATHS=0 \
	%{?__cc:CC="%__cc"} %{?__cxx:CXX="%__cxx"} \
	ascend ascxx pygtk tcltk models solvers

%install
rm -rf %{buildroot}
scons %{_smp_mflags} install

# Install menu entry for PyGTK interface, gtksourceview syntax highlighting, and MIME definition
pushd pygtk/gnome
install -m 644 -D %{name}.desktop %{buildroot}/%{_datadir}/applications/%{name}.desktop
install -m 644 -D %{name}.png %{buildroot}/%{_datadir}/icons/%{name}-app.png
install -m 644 -D %{name}.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/%{name}.png
install -m 644 -D %{name}.xml %{buildroot}/%{_datadir}/mime/packages/%{name}.xml
popd

# file-type icon for ascend models (double click should open in ASCEND)
pushd pygtk/glade
install -m 644 -D %{name}-doc-48x48.svg %{buildroot}/%{_datadir}/icons/text-x-%{name}-model.svg
popd

# language file for use with gedit
# gtksourceview-3.0?
pushd tools/gtksourceview-2.0
install -m 644 -D %{name}.lang %{buildroot}/%{gtksourceview_lang_file}
popd

# Install menu entry for Tcl/Tk interface
pushd tcltk/gnome
install -m 644 -D %{name}4.desktop %{buildroot}/%{_datadir}/applications/%{name}4.desktop
install -m 644 -D %{name}4.png %{buildroot}/%{_datadir}/icons/%{name}4-app.png
install -m 644 -D %{name}4.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/%{name}4.png
popd

# Fix .desktop files entries

desktop-file-edit                                       \
--set-icon="ascend-app"                                 \
--add-category="Science"                                \
--remove-key="Encoding"                                 \
--remove-key="Categories"                               \
%{buildroot}/%{_datadir}/applications/%{name}.desktop

desktop-file-edit                                       \
--set-icon="ascend-app"                                 \
--add-category="Science"                                \
--remove-key="Encoding"                                 \
--remove-key="Categories"                               \
%{buildroot}/%{_datadir}/applications/%{name}4.desktop

desktop-file-validate %{buildroot}/%{_datadir}/applications/%{name}.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/%{name}4.desktop

# Fixed execute permission
chmod +x %{buildroot}/%{_libdir}/%{name}/solvers/*.so
chmod +x %{buildroot}/%{_libdir}/%{name}/models/*/*.so
chmod +x %{buildroot}/%{_libdir}/%{name}/models/*/*/*.so
chmod +x %{buildroot}/%{_libdir}/libascend.so.1.0
chmod +x %{buildroot}/%{_libdir}/libascendtcl.so
chmod +x %{buildroot}/%{_libdir}/%{name}/models/johnpye/fprops/test/ph
chmod +x %{buildroot}/%{_libdir}/%{name}/models/johnpye/fprops/test/sat
chmod +x %{buildroot}/%{python_sitearch}/%{name}/_ascpy.so
chmod +x %{buildroot}/%{python_sitearch}/_fprops.so


# Fix non-executable-script warnings
for lib in %{buildroot}/%{_libdir}/%{name}/models/test/reverse_ad/modelgen.py; do
 sed '1{\@^#!/usr/bin/env python@d}' $lib > $lib.new &&
 touch -r $lib $lib.new &&
 mv $lib.new $lib
done

# Fix non-executable-script warnings
for lib in %{buildroot}/%{_libdir}/%{name}/models/johnpye/fprops/convcomp.py; do
 sed '1{\@^#!/usr/bin/env python@d}' $lib > $lib.new &&
 touch -r $lib $lib.new &&
 mv $lib.new $lib
done


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
#%defattr(644,root,root)
%doc INSTALL.txt LICENSE.txt
#%defattr(755,root,root)
%{_libdir}/%{name}/models
%{_libdir}/%{name}/solvers
%{_datadir}/mime/packages/%{name}.xml
%{gtksourceview_lang_file}
%{_datadir}/icons/text-x-%{name}-model.svg
#%defattr(755,root,root)
%{_libdir}/libascend.so.*
%dir %{python_sitearch}/%{name}
#%defattr(755,root,root)
%{_bindir}/%{name}
%{python_sitearch}/%{name}/_ascpy.so
#%defattr(644,root,root)
%{python_sitearch}/%{name}/*.py
%{python_sitearch}/%{name}/*.py[oc]
%{_datadir}/%{name}/glade
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/%{name}-app.png
%{_datadir}/icons/hicolor/64x64/%{name}.png
#%defattr(755,root,root)
%{python_sitearch}/_fprops.so
#%defattr(644,root,root)
%{python_sitearch}/fprops.py
%{python_sitearch}/fprops.py[oc]


%files tcltk
#%defattr(755,root,root)
%{_bindir}/%{name}4
%{_libdir}/libascendtcl.so
#%defattr(644,root,root)
%{_datadir}/%{name}/tcltk
%{_datadir}/applications/%{name}4.desktop
%{_datadir}/icons/%{name}4-app.png
%{_datadir}/icons/hicolor/64x64/%{name}4.png

%files devel
#%defattr(755,root,root)
%{_bindir}/%{name}-config
%{_includedir}/%{name}
%{_libdir}/lib*.so

%files doc
#%defattr(644,root,root)
%doc doc/book.pdf

%changelog
* Thu Jan 24 2013 Antonio Trande <sagitter@fedoraproject.org> 0.9.8-0.1.4424svn
- Fixed .desktop files entries
- Fixed source package origin
- Added BR texlive-collection-fontsrecommended,texlive-epstopdf,texlive-ulem 
  texlive-lm-math,desktop-file-utils  
- Fixed execute permission to various file
- Added Buildroot tag
- Fixed non-executable-script warnings

* Thu Jan 24 2013 Antonio Trande <sagitter@fedoraproject.org> 0.9.8-0.0.4424svn
- Initial package

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

