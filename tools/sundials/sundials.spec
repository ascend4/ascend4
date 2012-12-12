# This is a SPEC file to build SUNDIALS RPM for Fedora 17 and thereabouts.
# 
#
%define libname sundials

Summary:	Suite of nonlinear solvers
Name:		sundials
Version:	2.4.0
Release:	0%{?dist}
License:	BSD-style
Group:		System/Libraries
URL:		http://www.llnl.gov/casc/sundials/
Source0:	https://computation.llnl.gov/casc/sundials/download/code/%{name}-%{version}.tar.gz

# patch replaces config/ltmain.sh with a newer one.
#Patch0:         sundials-ltmain.patch

BuildRoot:	/var/tmp/%{name}-%{version}

%if 0%{?fedora_version}
%define makeinstall1 %makeinstall
BuildRequires: openmpi-devel
BuildRequires: gcc-gfortran
%else
%define makeinstall1 make prefix=$RPM_BUILD_ROOT%{_prefix} libdir=$RPM_BUILD_ROOT%{_libdir} includedir=$RPM_BUILD_ROOT%{_includedir} bindir=$RPM_BUILD_ROOT%{_bindir}      datadir=$RPM_BUILD_ROOT%{_datadir} mandir=$RPM_BUILD_ROOT%{_mandir} infodir=$RPM_BUILD_ROOT%{_infodir} install
%if 0%{?mandriva_version}
BuildRequires: libmpich1-devel mpicc
BuildRequires: gcc-gfortran
%else
%if 0%{?suse_version}
BuildRequires: mpich-devel
BuildRequires: gcc-fortran
%else
# fallback is xubuntu
BuildRequires: gfortran mpich
%endif
%endif
%endif

%description
SUNDIALS is a SUite of Non-linear DIfferential/ALgebraic equation Solvers
for use in writing mathematical software.

SUNDIALS was implemented with the goal of providing robust time integrators
and nonlinear solvers that can easily be incorporated into existing simulation
codes. The primary design goals were to require minimal information from the
user, allow users to easily supply their own data structures underneath the
solvers, and allow for easy incorporation of user-supplied linear solvers and
preconditioners. 

%package devel
Summary: Suite of nonlinear solvers (developer files)
Group:   System/Libraries
Requires: %{name} = %{version}
%description devel
SUNDIALS is a SUite of Non-linear DIfferential/ALgebraic equation Solvers
for use in writing mathematical software.

This package contains the developer files (.so file, header files)

%package doc
Summary: Suite of nonlinear solvers (documentation)
Group:   System/Libraries
%description doc
SUNDIALS is a SUite of Non-linear DIfferential/ALgebraic equation Solvers
for use in writing mathematical software.

This package contains the documentation files

%prep
%setup -q 
#%patch -p1

%build
./configure \
  CXX=g++ \
  CC=gcc \
  F77=gfortran \
  --enable-static=no \
  --enable-shared=yes \
  --prefix=%_prefix
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
%makeinstall1

# spot says better no .la files in RPMs
rm $RPM_BUILD_ROOT%_libdir/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc INSTALL_NOTES LICENSE README
%_libdir/*.so.[0-9].*
%_libdir/*.so.[0-9]

%files doc
%doc doc/cvode/cv_examples.pdf
%doc doc/cvode/cv_guide.pdf
%doc doc/kinsol/kin_examples.pdf
%doc doc/kinsol/kin_guide.pdf
%doc doc/cvodes/cvs_examples.pdf
%doc doc/cvodes/cvs_guide.pdf
%doc doc/ida/ida_examples.pdf
%doc doc/ida/ida_guide.pdf

%files devel
%_libdir/*.so
%_libdir/*.a
%_includedir
%_bindir/sundials-config

%changelog
* Wed Dec 12 2012 John Pye <john@curioussymbols.com> 2.5.0
- Updating for Fedora 17 (time passes...)

* Wed Jun 27 2007 John Pye <john@curioussymbols.com> 2.3.0
- Creating separate devel, doc and library packages.

* Sun Jun 24 2007 John Pye <john@curioussymbols.com> 2.3.0
- Fixed problem with creation of shared libraries (correction thanks to Andrey Romanenko in Debian).

* Sat Jun 23 2007 John Pye <john@curioussymbols.com> 2.3.0
- Ported to OpenSUSE Build Service, working on support for openSUSE alongside FC6, FC7.

* Thu Jul 27 2006 John Pye <john.pye@student.unsw.edu.au> 2.3.0-0
- First RPM spec created

