dnl
dnl This funtion takes the Var that tells you if a prog should be build, and the path of
dnl manpages with the basename of them, and generates the manopages the should be installed
dnl the manpages_retval variable. Dont'forget to fill the right var with before you call 
dnl this function the next time, and AC_SUBST the final var
dnl

AC_DEFUN([GENERATE_MANS_TO_INSTALL], [
	if test $1 == yes; then
		if test "$Generate_Langs" = "all"; then
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g'`
		else
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g' | grep $Generate_Langs `
			manpages_retval="`basename $2.1` $manpages_retval"
		fi
		manpages_retval=`echo $manpages_retval | tr -d '\n'`
	else
		manpages_retval=
	fi
])
dnl
dnl This funtion takes the Var that tells you if a prog should be build, and the path of
dnl manpages with the basename of them, and generates the manopages the should be installed
dnl the manpages_retval variable. Dont'forget to fill the right var with before you call 
dnl this function the next time, and AC_SUBST the final var
dnl

AC_DEFUN([GENERATE_MANS_TO_INSTALL], [
	if test $1 == yes; then
		if test "$Generate_Langs" = "all"; then
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g'`
		else
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g' | grep $Generate_Langs `
			manpages_retval="`basename $2.1` $manpages_retval"
		fi
		manpages_retval=`echo $manpages_retval | tr -d '\n'`
	else
		manpages_retval=
	fi
])
dnl
dnl This funtion takes the Var that tells you if a prog should be build, and the path of
dnl manpages with the basename of them, and generates the manopages the should be installed
dnl the manpages_retval variable. Dont'forget to fill the right var with before you call 
dnl this function the next time, and AC_SUBST the final var
dnl

AC_DEFUN([GENERATE_MANS_TO_INSTALL], [
	if test $1 == yes; then
		if test "$Generate_Langs" = "all"; then
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g'`
		else
			manpages_retval=`ls -1 $2.* | sed -e 's/.*\///g' | grep $Generate_Langs `
			manpages_retval="`basename $2.1` $manpages_retval"
		fi
		manpages_retval=`echo $manpages_retval | tr -d '\n'`
	else
		manpages_retval=
	fi
])
