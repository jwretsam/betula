dnl 
dnl Check for flex 
dnl AC_PROG_FLEX([VERSION=2.5.27])
dnl This macro define FLEX=flex if flex was found and FLEX=: otherwise
dnl
AC_DEFUN([AC_PROG_FLEX], [
if test "x$1" = "x" ; then 
	flex_required_version="2.5.27"
else
	flex_required_version="$1"
fi
AC_CHECK_PROG(have_prog_flex, [flex], [yes], [no])
if test "$have_prog_flex" = "yes" ; then 
	AC_MSG_CHECKING([for flex version >= $flex_required_version])
	flex_version=`flex --version | cut '-d ' -f 2`
	if test "$flex_version" = "$flex_required_version"  ; then 
		AC_MSG_RESULT([yes])
		FLEX=flex

	else

		AC_MSG_RESULT([no])
		FLEX=:
	fi
else
	AC_MSG_RESULT([no])
	FLEX=:
fi
AC_SUBST(FLEX)
])
	
