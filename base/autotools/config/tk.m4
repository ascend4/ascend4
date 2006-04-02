AC_DEFUN(ASC_TK,
[
#--------------------------------------------------------------------
# Look for Tk
#--------------------------------------------------------------------

TKINCLUDE=
TKLIB=
TKPACKAGE=

AC_ARG_WITH(tkconfig, AC_HELP_STRING([--without-tk], [Disable Tk])
AC_HELP_STRING([--with-tkconfig=path], [Set directory location of tkConfig.sh]), [with_tkconfig="$withval"], [with_tkconfig=])
AC_ARG_WITH(tk,
 [  --with-tk=path         Set location of Tk package],[
	TKPACKAGE="$withval"], [TKPACKAGE=yes])
AC_ARG_WITH(tkincl,[  --with-tkincl=path     Set location of Tk include directory],[
	TKINCLUDE="$ISYSTEM$withval"], [TKINCLUDE=])
AC_ARG_WITH(tklib,[  --with-tklib=path      Set location of Tk library directory],[
	TKLIB="-L$withval"], [TKLIB=])

# First, check for "--without-tk" or "--with-tk=no".
if test x"${TKPACKAGE}" = xno -o x"${with_alllang}" = xno; then 
AC_MSG_NOTICE([Disabling Tk])
else
AC_MSG_CHECKING([for Tk configuration])
# First check to see if --with-tkconfig was specified.
if test x"${with_tkconfig}" != x ; then
   if test -f "${with_tkconfig}/tkConfig.sh" ; then
      TKCONFIG=`(cd ${with_tkconfig}; pwd)`
   else
      AC_MSG_ERROR([${with_tk} directory doesn't contain tkConfig.sh])
   fi
fi
# check in a few common install locations
if test x"${TKCONFIG}" = x ; then
    for i in `ls -d /usr/lib 2>/dev/null` \
	    `ls -d /usr/local/lib 2>/dev/null` ; do
	if test -f "$i/tkConfig.sh" ; then
	    TKCONFIG=`(cd $i; pwd)`
	    break
	fi
    done
fi
if test x"${TKCONFIG}" = x ; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(found $TKCONFIG/tkConfig.sh)
    . $TKCONFIG/tkConfig.sh
    if test -z "$TKINCLUDE"; then
        TKINCLUDE=$ISYSTEM$TK_PREFIX/include
    fi
    if test -z "$TKLIB"; then
        TKLIB=$TK_LIB_SPEC
    fi
fi

if test -z "$TKINCLUDE"; then
   if test "x$TKPACKAGE" != xyes; then
	TKINCLUDE="$ISYSTEM$TKPACKAGE/include"
   fi
fi

if test -z "$TKLIB"; then
   if test "x$TKPACKAGE" != xyes; then
	TKLIB="-L$TKPACKAGE/lib -ltk"
   fi
fi

AC_MSG_CHECKING(for Tk header files)
if test -z "$TKINCLUDE"; then
AC_TRY_CPP([#include <tk.h>], , TKINCLUDE="")
if test -z "$TKINCLUDE"; then
	dirs="/usr/local/include /usr/include /opt/local/include"
	for i in $dirs ; do
		if test -r $i/tk.h; then
			AC_MSG_RESULT($i)
			TKINCLUDE="$ISYSTEM$i"
			break
		fi
	done
fi
if test -z "$TKINCLUDE"; then
    	AC_MSG_RESULT(not found)
fi
else
        AC_MSG_RESULT($TKINCLUDE)
fi

AC_MSG_CHECKING(for Tk library)
if test -z "$TKLIB"; then
dirs="/usr/local/lib /usr/lib /opt/local/lib"
for i in $dirs ; do
	if test -r $i/libtk.a; then
	    AC_MSG_RESULT($i)
	    TKLIB="-L$i -ltk"
	    break
	fi
done
if test -z "$TKLIB"; then
	AC_MSG_RESULT(not found)
fi
else
AC_MSG_RESULT($TKLIB)
fi

# Cygwin (Windows) needs the library for dynamic linking
case $host in
    *-*-cygwin* | *-*-mingw*) TKDYNAMICLINKING="$TKLIB";;
    *)TKDYNAMICLINKING="";;
esac
fi

AC_SUBST(TKINCLUDE)
AC_SUBST(TKLIB)
AC_SUBST(TKDYNAMICLINKING)

])

