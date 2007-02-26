dnl ----------------------------------------------------
dnl AC_OPTIONS_ZLIB
dnl
dnl Add support for --with-zlib command-line parameter.
dnl PREFIX may be a directory prefix where zlib is installed, e.g. /usr/local
dnl or may be one of the following special keywords:
dnl    peer - check zlib in peer directory (not supported, does anyone use it?)
dnl    sys - use system zlib
dnl ----------------------------------------------------
AC_DEFUN([AC_OPTIONS_ZLIB],
[
	AC_ARG_WITH(
		[zlib],
		AS_HELP_STRING(
			[--with-zlib=PREFIX],
			[use zlib in PREFIX]),
		[ac_zlib="$withval"],
		[ac_zlib=yes])
])


dnl ----------------------------------------------------
dnl AC_CHECK_ZLIB([MIN_ZLIB_VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl check if zlib is on the system
dnl ----------------------------------------------------
AC_DEFUN([AC_CHECK_ZLIB],
[
ac_zver_max="`echo $1 | cut -d. -f1`"
ac_zver_mid="`echo $1 | cut -d. -f2`"
ac_zver_min="`echo $1 | cut -d. -f3`"

case "$ac_zlib" in
no)
	ifelse([$3], , :, [$3])
	;;
yes | sys)
	;;
*)
	ac_zlib_prefix="$ac_zlib"
esac

	if test -n "$ac_zlib_prefix"; then
		ac_tmp_CPPFLAGS="$CPPFLAGS"
		ac_tmp_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$CPPFLAGS -I$ac_zlib_prefix/include"
		LDFLAGS="-L$ac_zlib_prefix/lib"
	fi
	ac_tmp_LIBS="$LIBS"
	LIBS="-lz $LIBS"
	AC_MSG_CHECKING([for zlib >= $1])
	AC_RUN_IFELSE([
		AC_LANG_PROGRAM([[
			#include <zlib.h>
			#include <stdio.h>
		]], [[
			char *zver = zlibVersion();
			FILE *f=fopen("conftestval", "w");
			if (!f) return 1;
			fprintf(f, "%s",
				zver[0] > '$ac_zver_max' ||
				(zver[0] == '$ac_zver_max' &&
				(zver[2] > '$ac_zver_mid' ||
				(zver[2] == '$ac_zver_mid' &&
				zver[4] >= '$ac_zver_min'))) ? "yes" : "no");
			fclose(f);
			f=fopen("conftestver", "w");
			if (f) {
				fprintf(f, "%s", ZLIB_VERSION);
				fclose(f);
			}
		]])
	], [
		if test -f conftestval; then
	        	result=`cat conftestval`
		else
			result=no
		fi
		if test x$result = xyes; then
			if test -f conftestver; then
				ZLIB_VERSION=`cat conftestver`
				z_version=" (version $ZLIB_VERSION)"
			else
				z_version=""
			fi
		fi
		AC_MSG_RESULT($result$z_version)
	], [
		result=no
		AC_MSG_RESULT($result)
	], [
		result=no
		z_version=''
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <zlib.h>
			]], [[
				const char zver[] = "\nZLIB_VERSION_START" ZLIB_VERSION "ZLIB_VERSION_END\n";
				zlibVersion();
			]])
		], [
			ZLIB_VERSION=`grep -a '^ZLIB_VERSION_START.*ZLIB_VERSION_END$' conftest$ac_exeext | sed 's/^ZLIB_VERSION_START\(.*\)ZLIB_VERSION_END$/\1/'`
			ac_cross_zver_max="`echo $ZLIB_VERSION | cut -d. -f1`"
			ac_cross_zver_mid="`echo $ZLIB_VERSION | cut -d. -f2`"
			ac_cross_zver_min="`echo $ZLIB_VERSION | cut -d. -f3`"
			if test "$ac_cross_zver_max" -gt "$ac_zver_max"; then
				result=yes
			elif test "$ac_cross_zver_max" -eq "$ac_zver_max"; then
				if test "$ac_cross_zver_mid" -gt "$ac_zver_mid"; then
					result=yes
				elif "$ac_cross_zver_mid" -eq "$ac_zver_mid"; then
					if test "$ac_cross_zver_min" -ge "$ac_zver_min"; then
						result=yes
					fi
				fi
			fi
			if test x$result = xyes; then
				z_version=" (version $ZLIB_VERSION)"
			fi
		])
		AC_MSG_RESULT($result$z_version)
	])
	if test x$result = xno; then
		if test "${ac_tmp_CPPFLAGS+set}" = set; then
			CPPFLAGS="$ac_tmp_CPPFLAGS"
		fi
		if test "${ac_tmp_LDFLAGS+set}" = set; then
			LDFLAGS="$ac_tmp_LDFLAGS"
		fi
		LIBS="$ac_tmp_LIBS"
		ifelse([$3], , :, [$3])
	else
		ifelse([$2], , :, [$2])
	fi
])
