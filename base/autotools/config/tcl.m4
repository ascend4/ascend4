AC_DEFUN(ASC_TCL,
[
#--------------------------------------------------------------------
# Look for Tcl
#--------------------------------------------------------------------

TCLINCLUDE=
TCLLIB=
TCLPACKAGE=

AC_ARG_WITH(tclconfig, AC_HELP_STRING([--without-tcl], [Disable Tcl])
AC_HELP_STRING([--with-tclconfig=path], [Set directory location of tclConfig.sh]), [with_tclconfig="$withval"], [with_tclconfig=])
AC_ARG_WITH(tcl,
 [  --with-tcl=path         Set location of Tcl package],[
	TCLPACKAGE="$withval"], [TCLPACKAGE=yes])
AC_ARG_WITH(tclincl,[  --with-tclincl=path     Set location of Tcl include directory],[
	TCLINCLUDE="$ISYSTEM$withval"], [TCLINCLUDE=])
AC_ARG_WITH(tcllib,[  --with-tcllib=path      Set location of Tcl library directory],[
	TCLLIB="-L$withval"], [TCLLIB=])

# First, check for "--without-tcl" or "--with-tcl=no".
if test x"${TCLPACKAGE}" = xno -o x"${with_alllang}" = xno; then 
AC_MSG_NOTICE([Disabling Tcl])
else
AC_MSG_CHECKING([for Tcl configuration])
# First check to see if --with-tclconfig was specified.
if test x"${with_tclconfig}" != x ; then
   if test -f "${with_tclconfig}/tclConfig.sh" ; then
      TCLCONFIG=`(cd ${with_tclconfig}; pwd)`
   else
      AC_MSG_ERROR([${with_tcl} directory doesn't contain tclConfig.sh])
   fi
fi
# check in a few common install locations
if test x"${TCLCONFIG}" = x ; then
    for i in `ls -d /usr/lib 2>/dev/null` \
	    `ls -d /usr/local/lib 2>/dev/null` ; do
	if test -f "$i/tclConfig.sh" ; then
	    TCLCONFIG=`(cd $i; pwd)`
	    break
	fi
    done
fi
if test x"${TCLCONFIG}" = x ; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(found $TCLCONFIG/tclConfig.sh)
    . $TCLCONFIG/tclConfig.sh
    if test -z "$TCLINCLUDE"; then
        TCLINCLUDE=$ISYSTEM$TCL_PREFIX/include
    fi
    if test -z "$TCLLIB"; then
        TCLLIB=$TCL_LIB_SPEC
    fi
fi

if test -z "$TCLINCLUDE"; then
   if test "x$TCLPACKAGE" != xyes; then
	TCLINCLUDE="$ISYSTEM$TCLPACKAGE/include"
   fi
fi

if test -z "$TCLLIB"; then
   if test "x$TCLPACKAGE" != xyes; then
	TCLLIB="-L$TCLPACKAGE/lib -ltcl"
   fi
fi

AC_MSG_CHECKING(for Tcl header files)
if test -z "$TCLINCLUDE"; then
AC_TRY_CPP([#include <tcl.h>], , TCLINCLUDE="")
if test -z "$TCLINCLUDE"; then
	dirs="/usr/local/include /usr/include /opt/local/include"
	for i in $dirs ; do
		if test -r $i/tcl.h; then
			AC_MSG_RESULT($i)
			TCLINCLUDE="$ISYSTEM$i"
			break
		fi
	done
fi
if test -z "$TCLINCLUDE"; then
    	AC_MSG_RESULT(not found)
fi
else
        AC_MSG_RESULT($TCLINCLUDE)
fi

AC_MSG_CHECKING(for Tcl library)
if test -z "$TCLLIB"; then
dirs="/usr/local/lib /usr/lib /opt/local/lib"
for i in $dirs ; do
	if test -r $i/libtcl.a; then
	    AC_MSG_RESULT($i)
	    TCLLIB="-L$i -ltcl"
	    break
	fi
done
if test -z "$TCLLIB"; then
	AC_MSG_RESULT(not found)
fi
else
AC_MSG_RESULT($TCLLIB)
fi

# Cygwin (Windows) needs the library for dynamic linking
case $host in
    *-*-cygwin* | *-*-mingw*) TCLDYNAMICLINKING="$TCLLIB";;
    *)TCLDYNAMICLINKING="";;
esac
fi

NOTCLCONFIG=
if test x"${TCLCONFIG}" = x ; then
NOTCLCONFIG="# tclConfig.sh dir not found"
fi
AC_SUBST(TCLINCLUDE)
AC_SUBST(NOTCLCONFIG)
AC_SUBST(TCLCONFIG)
AC_SUBST(TCLLIB)
AC_SUBST(TCLDYNAMICLINKING)

])

