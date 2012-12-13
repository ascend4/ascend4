Name:		ipopt
Summary:	Large-scale optimisation solver
Version:	3.10.3
Release:	0%{?dist}

%define mumpsversion 4.10.0
%define metisversion 4.0.3

Patch0:	ipopt-addlibs.patch
Source0:	http://www.coin-or.org/download/source/Ipopt/Ipopt-%{version}.tgz
Source1:	http://mumps.enseeiht.fr/MUMPS_%{mumpsversion}.tar.gz
Source2:        http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/OLD/metis-%{metisversion}.tar.gz
#Patch0:     Ipopt-hsl-shared.patch

URL:		https://projects.coin-or.org/Ipopt
License:	CPL
Group:		Development/Libraries
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

BuildRequires: gcc-c++ blas-devel lapack-devel gcc-gfortran
Obsoletes: Ipopt

%description
IPOPT (Interior Point Optimizer, pronounced 'I-P-Opt') is an open source 
software package for large-scale nonlinear optimization. It can be used to solve
general nonlinear programming problems of the form

   min f(x)
    s.t. g_L <= g(x) <= g_U
    and x_L <= x <= x_U

where x is a vector of n real variables.

%package devel
Summary: Large-scale optimisation solver (developer files)
Group:   Development/Libraries
Requires: %{name} lapack-devel blas-devel gcc-gfortran
Obsoletes: Ipopt-devel
%description devel
This package contains the header files and developer libraries
for IPOPT, for use by people building software that depends on
IPOPT.

%package doc
Summary: Large-scale optimisation solver (documentation)
Group:   Development/Libraries
Obsoletes: Ipopt-doc
%description doc
This package contains the user manual for the IPOPT large-scale
optimisation solver. The PDF file can be found in
%_datadir/%{name}-%{version}/doc

%prep
%setup -q -n Ipopt-%{version}
%patch0 -p1
pwd
tar zxf ../../SOURCES/MUMPS_%{mumpsversion}.tar.gz
mv MUMPS_%{mumpsversion} ThirdParty/Mumps/MUMPS
tar zxf ../../SOURCES/metis-%{metisversion}.tar.gz
mv metis-%{metisversion} ThirdParty/Metis/metis-4.0

%build
#BuildTools/run_autotools
%{?__cc:CC="%__cc"} %{?__cxx:CXX="%__cxx"} %configure
make %{?_smp_mflags} %{?__cc:CC="%__cc"} %{?__cxx:CXX="%__cxx"}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT INSTALL="%{__install} -p" install

# spot says we don't want .la files in RPMs:
rm %{buildroot}/%{_libdir}/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig 2>/dev/null

%postun
/sbin/ldconfig 2>/dev/null

%files
%defattr(-,root,root)
%doc README LICENSE ChangeLog
%_libdir/*.so.[0-9].*
%_libdir/*.so.[0-9]

%files devel
%defattr(-,root,root)
%_includedir/coin
%_libdir/*.so
%_libdir/pkgconfig

%files doc
%defattr(-,root,root)
%doc Ipopt/doc/documentation.pdf
%doc ThirdParty/Mumps/MUMPS/doc/userguide_%{mumpsversion}.pdf
/usr/share/doc
/usr/share/coin/doc/Ipopt

%changelog
* Thu Dec 13 2012 John Pye <john@curioussymbols.com> 3.10.3
- Updated version.
- Included METIS as part of source package (in addition to MUMPS).

* Fri Jun 20 2008 John Pye <john@curioussymbols.com> 3.4.1
- Updated version
- Removed %_libdir/*.txt from file list
- Moved %_libdir/*.so.[0-9] from file list
- Added devel dependencies on gcc-gfortran lapack-devel and blas-devel (because of funny way that IPOPT is linked)

* Sat Jan 05 2008 John Pye <john@curioussymbols.com> 3.3.4
- Updated version

* Sun Jul 22 2007 John Pye <john@curioussymbols.com> 3.3.2
- Updated version.
- Renamed packages to lowercase ipopt, with 'Obsoletes:' for old names

* Tue Jun 26 2007 John Pye <john@curioussymbols.com> 3.3.1
- First version

# -*- coding: UTF-8 -*-

