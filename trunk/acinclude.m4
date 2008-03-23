dnl ---------------------------------------------------------------------------
dnl AM_WXCONFIG_LARGEFILE()
dnl
dnl Test that wxWidgets is built with support for large-files. If not
dnl configure is terminated.
dnl ---------------------------------------------------------------------------
AC_DEFUN([AM_WXCONFIG_LARGEFILE],
[
	AC_LANG_PUSH(C++)
	
	dnl Backup current flags and setup flags for testing
	__CPPFLAGS=${CPPFLAGS}
	CPPFLAGS=${WX_CPPFLAGS}
	
	AC_MSG_CHECKING(that wxWidgets has support for large files)
	AC_PREPROC_IFELSE([
		#include <wx/wx.h>

		int main() {
		#if !HAVE_LARGEFILE_SUPPORT && !defined(__WXMSW__)
			#error No LargeFile support!;
		#endif
			exit(0);
		}
	], , NO_LF="true")

	if test "x${NO_LF}" != "x";
	then
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([
		Support for large files in wxWidgets is required by aMule.
		To continue you must recompile wxWidgets with support for 
		large files enabled.
		])
	else
		AC_MSG_RESULT(yes)
	fi

	dnl Restore backup'd flags
	CPPFLAGS=${__CPPFLAGS}	

	AC_LANG_POP(C++)
])


dnl --------------------------------------------------------------------------
dnl CCache support
dnl --------------------------------------------------------------------------

AC_DEFUN([CHECK_CCACHE],
	[
	if test x$ccache_prefix == x ; then
		ccache_full=$(which ccache)
		ccache_prefix=$(dirname ${ccache_full})
	fi
	$ccache_prefix/ccache -V > /dev/null 2>&1
	CCACHE=$?
	if test "$CCACHE" != 0; then
		result="no"
	else
		result="yes"
	fi
     AC_MSG_CHECKING([for ccache presence])
	AC_MSG_RESULT($result)
	AC_SUBST(CCACHE)
	AC_SUBST(ccache_prefix)
	])


AC_DEFUN([AM_OPTIONS_CCACHE_PFX],
[
	AC_ARG_WITH(
		[ccache-prefix],
		[AS_HELP_STRING(
			[--with-ccache-prefix=PREFIX],
			[prefix where ccache is installed])],
		[ccache_prefix="$withval"],
		[ccache_prefix=""])
])


dnl ----------------------------------------------------
dnl CHECK_BFD
dnl check if bfd.h is on the system and usable
dnl ----------------------------------------------------
AC_DEFUN([CHECK_BFD],
[
AC_MSG_CHECKING([for bfd])
__LIBS=$LIBS
LIBS="-lbfd -liberty $LIBS"
AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#include <ansidecl.h>
			#include <bfd.h>
		]], [[
			char *dummy = bfd_errmsg(bfd_get_error());
		]])
	], [
		AC_MSG_RESULT([yes])
		BFD_FLAGS="-DHAVE_BFD"
		BFD_LIB="-lbfd -liberty"
	], [
		AC_MSG_RESULT([no])
		AC_MSG_WARN([
	bfd.h not found or unusable, please install binutils development
	package if you are a developer or want to help testing aMule])
	])
LIBS=${__LIBS}
AC_SUBST(BFD_FLAGS)
AC_SUBST(BFD_LIB)
])


dnl ----------------------------------------------------
dnl CHECK_AUTOPOINT
dnl check if autopoint is installed and fail if not
dnl ----------------------------------------------------
AC_DEFUN([CHECK_AUTOPOINT],
[

AC_MSG_CHECKING([for autopoint])

autopoint_version=`autopoint --version | head -n 1 | sed -e 's/.*[[^0-9.]]\([[0-9]]\{1,\}\(\.[[0-9]]\{1,\}\)\{1,2\}\)[[^0-9.]]*/\1/'`
if test x$autopoint_version != x; then
	result="yes"
else
	result="no"
fi

HAVE_GETTEXT=$result

AC_MSG_RESULT($result ($autopoint_version))
if test x$result = xno; then
	AC_MSG_NOTICE([You need to install GNU gettext/gettext-tools to compile aMule with i18n support])
fi
AC_SUBST(HAVE_GETTEXT)
])


dnl ----------------------------------------------------
dnl CHECK_FLEX_EXTENDED
dnl check if flex can produce header files
dnl ----------------------------------------------------
AC_DEFUN([CHECK_FLEX_EXTENDED],
[

AC_MSG_CHECKING([for extended flex capabilities])

extended_flex=`flex --help | grep header-file`
if test x"$extended_flex" != x""; then
	result="yes"
else
	result="no"
fi

HAVE_FLEX_EXTENDED=$result

AC_MSG_RESULT($result)

if test x$result = xno; then
	AC_MSG_NOTICE([Your flex version doesn't support --header-file flag. This is not critical, but an upgrade is recommended ])
fi

AC_SUBST(HAVE_FLEX_EXTENDED)

])


dnl ----------------------------------------------------
dnl CHECK_EXCEPTIONS
dnl Checks for broken exception-handling. This is needed
dnl because exception handling is broken for some archs/
dnl compilers.
dnl ----------------------------------------------------
AC_DEFUN([CHECK_EXCEPTIONS],
[
	AC_MSG_CHECKING([for exception-handling])

	AC_LANG_PUSH(C++)
	AC_RUN_IFELSE([
		AC_LANG_PROGRAM([],
		[[
			try {
				throw 1;
			} catch (int) {
				return 0;
			}
			return 1;
		]])
	], [
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Exception handling does not work. Broken compiler?])
	], [
		AC_MSG_RESULT([undeterminable])
		AC_MSG_WARN([
	Cross-compilation detected, so exception handling cannot be tested.
	Note that broken exception handling in your compiler may lead to
	unexpected crashes.])
	])
	AC_LANG_POP(C++)
])


dnl ---------------------------------------------------------------------------
dnl GENERATE_MANS_TO_INSTALL(TESTNAME, BASENAMEPATH)
dnl
dnl This function will generate the list of manpages to be installed.
dnl
dnl TESTNAME is the name of a variable that'll evaluate to yes if this
dnl set of manpages need installing. The list of files will be returned in
dnl the TESTNAME_MANPAGES variable.
dnl
dnl BASENAMEPATH is the path and basename of the manpages we test for, relative
dnl to the package root (top_srcdir)
dnl ---------------------------------------------------------------------------
AC_DEFUN([GENERATE_MANS_TO_INSTALL],
[
	if test "$[]$1" == "yes"; then
		if test "$LINGUAS" = ""; then
			$1_MANPAGES=`ls -1 ${srcdir}/$2.* | sed -e 's:.*/::g'`
		else
			$1_MANPAGES=`ls -1 ${srcdir}/$2.* | sed -e 's:.*/::g' | grep $Generate_Langs `
			$1_MANPAGES="`basename $2.1` $[]$1_MANPAGES"
		fi
		$1_MANPAGES=`echo $[]$1_MANPAGES | tr -d '\n'`
	else
		$1_MANPAGES=
	fi
	AC_SUBST($1_MANPAGES)
])


dnl ---------------------------------------------------------------------------
dnl AC_CHECK_REGEX([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl This function will test the existance of a POSIX compliant regex library.
dnl ---------------------------------------------------------------------------
AC_DEFUN([AC_CHECK_REGEX],
[
	AC_MSG_CHECKING([for a POSIX compliant regex library])
	regex_found=no
	for lib in '' -lgnurx -lregex; do
		saved_LIBS="$LIBS"
		LIBS="$lib $LIBS"
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <regex.h>
			]], [[
				regex_t preg;
				regcomp(&preg, "", REG_EXTENDED);
				regmatch_t *pmatch;
				regexec(&preg, "", 0, pmatch, 0);
				regfree(&preg);
			]])
		], [
			LIBS="$saved_LIBS"
			regex_found=yes
			break;
		], [
			LIBS="$saved_LIBS"
		])
	done
	AC_MSG_RESULT([$regex_found])
	AS_IF([test $regex_found = yes],
	[
		REGEX_LIB=$lib
		$1
	], [$2])
	AC_SUBST([REGEX_LIB])
])
