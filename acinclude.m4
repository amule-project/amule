#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
#
# Any parts of this program derived from the xMule, lMule or eMule project,
# or contributed by third-party developers are copyrighted by their
# respective authors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
#

dnl ---------------------------------------------------------------------------
dnl Helper functions
dnl ---------------------------------------------------------------------------
m4_pattern_forbid(MULE_)dnl Check for unexpanded *MULE_* macros
m4_pattern_allow(AMULE_)dnl Allow the *AMULE_* names

dnl MULE_APPEND(VARNAME, VALUE)
AC_DEFUN([MULE_APPEND], [$1="$$1 $2"])

dnl MULE_PREPEND(VARNAME, VALUE)
AC_DEFUN([MULE_PREPEND], [$1="$2 $$1"])

dnl MULE_ADDFLAG(FLAGTYPE, VALUE)
AC_DEFUN([MULE_ADDFLAG], [MULE_APPEND([MULE[]$1[]FLAGS], [$2])])

dnl MULE_ADDCCXXFLAG(VALUE)
AC_DEFUN([MULE_ADDCCXXFLAG],
[
	MULE_ADDFLAG([C], [$1])
	MULE_ADDFLAG([CXX], [$1])
])

dnl MULE_BACKUP(VAR)
AC_DEFUN([MULE_BACKUP], [mule_backup_$1="$$1"])

dnl MULE_RESTORE(VAR)
AC_DEFUN([MULE_RESTORE], [$1="$mule_backup_$1"])

dnl Helper macro for MULE_IF
m4_define([__mule_if_helper],
[m4_if( [$#], 0,,
	[$#], 1, [m4_ifvaln([$1], [m4_n([else])  $1])],
	[m4_n([elif $1; then])  m4_ifvaln([$2], [$2], :)])dnl
m4_if(m4_eval([$# > 2]), 1, [$0(m4_shiftn(2, $@))])])

dnl MULE_IF(CONDITION, [IF-TRUE] [, ELIF-CONDITION, [IF-TRUE]]... [, ELSE-BRANCH])
m4_define([MULE_IF],
[m4_if( [$#], 0,,
	[$#], 1,,
	[m4_n([if $1; then])  m4_ifval([$2],[$2], :)
m4_if(m4_eval([$# > 2]), 1, [__mule_if_helper(m4_shiftn(2, $@))])m4_n([fi])])])

dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_SYSTEM
dnl
dnl Checks host system type, and sets system-specific flags accordingly.
dnl Sets $SYS to the name of the host os.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_SYSTEM],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl

	case "${host_os}" in
	darwin*)
		SYS=darwin
		MULECPPFLAGS="-no-cpp-precomp -D_INTL_REDIRECT_MACROS -DNOPCH";
		MULELDFLAGS="-bind_at_load"
		touch src/Scanner.cpp
		;;
	openbsd*) 
		SYS=openbsd
		LIBS="$LIBS -L/usr/local/lib"
		X11LIBS="-lX11 -L/usr/X11R6/lib"
		MULECPPFLAGS="-D__OPENBSD__"
		;;
	*cygwin* | *mingw32*)
		SYS=win32
		MULECPPFLAGS="-DNOMINMAX"
		;;
	solaris*)
		SYS=solaris
		RESOLV_LIB="-lresolv -lnsl"
		X11LIBS="-lX11"
		LIBS="$LIBS -lrt"
		;;
	*netbsd*)
		SYS=netbsd
		# Now this is against autoconf recommendation that configure should not modify CPPFLAGS and LDFLAGS
		# However, these values in NetBSD are required even to run the tests, and this is the easiest way to do it.
		# Still, we prepend them, instead of adding, so the user may override them.
		MULE_PREPEND([CPPFLAGS], [-I/usr/pkg/include])
		MULE_PREPEND([LDFLAGS], [-R/usr/pkg/lib -L/usr/pkg/lib])
		;;
	*irix*)
		SYS=irix
		MULECPPFLAGS="-D__IRIX__"
		;;
	*)
		SYS=unknown
		;;
	esac

	# -lpthread is needed by Debian but FreeBSD < 5 doesn't support it
	AS_IF([test ${SYS:-unknown} != win32],
	[
		AC_MSG_CHECKING([if this is a FreeBSD 4 or earlier system])
		AS_IF([test x"`uname -s`" = xFreeBSD && test 0`uname -r | cut -c 1` -lt 5],
		[
			MULE_ADDFLAG([LD], [-pthread])
			AC_MSG_RESULT(yes)
		], [
			MULE_ADDFLAG([LD], [-lpthread])
			AC_MSG_RESULT(no)
		])
	])

AC_SUBST([RESOLV_LIB])dnl
AC_SUBST([X11LIBS])dnl
AC_SUBST([MULECPPFLAGS])dnl
AC_SUBST([MULECFLAGS])dnl
AC_SUBST([MULECXXFLAGS])dnl
AC_SUBST([MULELDFLAGS])dnl
AC_SUBST([MULERCFLAGS])dnl
])

dnl ---------------------------------------------------------------------------
dnl _MULE_CHECK_DEBUG_FLAG
dnl ---------------------------------------------------------------------------
AC_DEFUN([_MULE_CHECK_DEBUG_FLAG],
[AC_REQUIRE([MULE_CHECK_GLIBCXX])dnl

	AC_ARG_ENABLE(
		[debug],
		[AS_HELP_STRING([--disable-debug], [disable additional debugging output])],
		[USE_DEBUG=${enableval:-yes}], [USE_DEBUG=yes])

	AS_IF([test $USE_DEBUG = yes],
	[
		MULE_ADDFLAG([CPP], [-D__DEBUG__])
		MULE_ADDCCXXFLAG([-g])
		AS_IF([test ${GLIBCXX:-no} = yes],	[MULE_ADDFLAG([CPP], [-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC])])
		AS_IF([test ${GCC:-no} = yes],		[MULE_ADDCCXXFLAG([-W -Wall -Wshadow -Wundef -ggdb -fno-inline -fmessage-length=0])])
		AS_IF([test ${SYS:-unknown} = win32],	[MULE_ADDFLAG([RC], [-D__DEBUG__])])
	], [
		AS_IF([test ${GCC:-no} = yes], [MULE_ADDCCXXFLAG([-W -Wall -Wshadow -Wundef])])
	])
])

dnl ---------------------------------------------------------------------------
dnl _MULE_CHECK_PROFILE_FLAG
dnl ---------------------------------------------------------------------------
AC_DEFUN([_MULE_CHECK_PROFILE_FLAG],
[
	AC_ARG_ENABLE(
		[profile],
		[AS_HELP_STRING([--enable-profile], [enable code profiling])],
		[USE_PROFILE=${enableval:-no}], [USE_PROFILE=no])

	AS_IF([test $USE_PROFILE = yes],
	[
		MULE_ADDCCXXFLAG([-pg])
		MULE_ADDFLAG([LD], [-pg])
	])
])

dnl ---------------------------------------------------------------------------
dnl _MULE_CHECK_OPTIMIZE_FLAG
dnl ---------------------------------------------------------------------------
AC_DEFUN([_MULE_CHECK_OPTIMIZE_FLAG],
[
	AC_ARG_ENABLE(
		[optimize],
		[AS_HELP_STRING([--enable-optimize], [enable code optimization])],
		[USE_OPTIMIZE=${enableval:-no}], [USE_OPTIMIZE=no])

	AS_IF([test $USE_OPTIMIZE = yes], [MULE_ADDCCXXFLAG([-O2])])
])

dnl ---------------------------------------------------------------------------
dnl MULE_COMPILATION_FLAGS
dnl
dnl Checks type of compilation requested by user, and sets various flags
dnl accordingly.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_COMPILATION_FLAGS],
[
	_MULE_CHECK_DEBUG_FLAG
	_MULE_CHECK_OPTIMIZE_FLAG
	_MULE_CHECK_PROFILE_FLAG

	AC_MSG_CHECKING([if the applications should be statically linked])
	AC_ARG_ENABLE(
		[static],
		[AS_HELP_STRING([--enable-static], [produce a statically linked executable])],
		[AS_IF([test ${enableval:-no} = yes], [MULE_ADDFLAG([LD], [-static])])])
	AC_MSG_RESULT(${enableval:-no})

	MULE_ADDFLAG([CPP], [-DUSE_WX_EXTENSIONS])
])

dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_GLIBCXX
dnl
dnl Checks whether we use the GNU C++ Library.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_GLIBCXX],
[dnl
AC_REQUIRE([AC_PROG_EGREP])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl
AC_LANG_ASSERT([C++])dnl

	AC_MSG_CHECKING([if we're using the GNU C++ library])
	AC_PREPROC_IFELSE([
		AC_LANG_SOURCE([[
			#include <string>
			#ifndef __GLIBCXX__
			#error Non-GNU C++ library found.
			#endif
		]])
	], [GLIBCXX=yes], [GLIBCXX=no])
	AC_MSG_RESULT([$GLIBCXX])
])

dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_WX_SUPPORTS_LARGEFILE
dnl
dnl Test that wxWidgets is built with support for large-files. If not
dnl configure is terminated.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_WX_SUPPORTS_LARGEFILE],
[AC_LANG_ASSERT([C++])dnl

	dnl Backup current flags and setup flags for testing
	MULE_BACKUP([CPPFLAGS])
	MULE_APPEND([CPPFLAGS], [$WX_CPPFLAGS])

	AC_MSG_CHECKING([that wxWidgets has support for large files])
	AC_PREPROC_IFELSE([
		AC_LANG_SOURCE([[
			#include <wx/wx.h>
			#if !HAVE_LARGEFILE_SUPPORT && !defined(_LARGE_FILES) && !defined(__WXMSW__)
				#error No LargeFile support!
			#endif
		]])
	], [
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([
	Support for large files in wxWidgets is required by aMule.
	To continue you must recompile wxWidgets with support for 
	large files enabled.])
	])

	dnl Restore backup'd flags
	MULE_RESTORE([CPPFLAGS])
])


dnl --------------------------------------------------------------------------
dnl MULE_CHECK_CCACHE
dnl
dnl Checks if ccache is requested and available, and makes use of it
dnl --------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_CCACHE],
[
	AC_ARG_ENABLE([ccache],
		[AS_HELP_STRING([--enable-ccache], [enable ccache support for fast recompilation])])

	AC_ARG_WITH([ccache-prefix],
		[AS_HELP_STRING([--with-ccache-prefix=PREFIX], [prefix where ccache is installed])])

	AC_MSG_CHECKING([whether ccache support should be added])
	AC_MSG_RESULT([${enable_ccache:-no}])

	AS_IF([test ${enable_ccache:-no} = yes], [
		AC_MSG_CHECKING([for ccache presence])
		AS_IF([test -z "$with_ccache_prefix"], [
			ccache_full=`which ccache`
			with_ccache_prefix=`dirname ${ccache_full}`
		])
		AS_IF([$with_ccache_prefix/ccache -V >/dev/null 2>&1], [
			AC_MSG_RESULT([yes])
			CC="$with_ccache_prefix/ccache $CC"
			CXX="$with_ccache_prefix/ccache $CXX"
			BUILD_CC="$with_ccache_prefix/ccache $BUILD_CC"
		], [
			enable_ccache=no
			AC_MSG_RESULT([no])
		])
	])
])


dnl ----------------------------------------------------
dnl MULE_CHECK_BFD
dnl check if bfd.h is on the system and usable
dnl ----------------------------------------------------
AC_DEFUN([MULE_CHECK_BFD],
[AC_REQUIRE([MULE_CHECK_NLS])dnl

	AC_MSG_CHECKING([for bfd])
	result=no
	for bfd_ldadd in "" "${LIBINTL}"; do
		MULE_BACKUP([LIBS])
		MULE_BACKUP([LDFLAGS])
		MULE_PREPEND([LIBS], [-lbfd -liberty ${bfd_ldadd} ${ZLIB_LIBS}])
		MULE_APPEND([LDFLAGS], [${ZLIB_LDFLAGS}])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <ansidecl.h>
				#include <bfd.h>
			]], [[
				char *dummy = bfd_errmsg(bfd_get_error());
			]])
		], [
			result=yes
			BFD_CPPFLAGS="-DHAVE_BFD"
			BFD_LIBS="-lbfd -liberty ${bfd_ldadd}"
			MULE_RESTORE([LIBS])
			MULE_RESTORE([LDFLAGS])
			break
		])
		MULE_RESTORE([LIBS])
		MULE_RESTORE([LDFLAGS])
	done

	AC_MSG_RESULT([$result])

	AS_IF([test $result = no],
		[AC_MSG_WARN([
	bfd.h not found or unusable, please install binutils development
	package if you are a developer or want to help testing aMule])
	])])

AC_SUBST([BFD_CPPFLAGS])dnl
AC_SUBST([BFD_LIBS])dnl
])


dnl ----------------------------------------------------
dnl MULE_CHECK_FLEX_EXTENDED
dnl check if flex can produce header files
dnl ----------------------------------------------------
AC_DEFUN([MULE_CHECK_FLEX_EXTENDED],
[
	AC_MSG_CHECKING([for extended flex capabilities])

	extended_flex=`flex --help | grep header-file`
	AS_IF([test -n "$extended_flex"], [HAVE_FLEX_EXTENDED=yes], [HAVE_FLEX_EXTENDED=no])
	AC_MSG_RESULT($HAVE_FLEX_EXTENDED)

	AS_IF([test $HAVE_FLEX_EXTENDED = no], [AC_MSG_NOTICE([Your flex version doesn't support --header-file flag. This is not critical, but an upgrade is recommended])])
])


dnl ----------------------------------------------------
dnl MULE_CHECK_EXCEPTIONS
dnl Checks for broken exception-handling. This is needed
dnl because exception handling is broken for some archs/
dnl compilers.
dnl ----------------------------------------------------
AC_DEFUN([MULE_CHECK_EXCEPTIONS],
[AC_LANG_ASSERT([C++])dnl

	AC_MSG_CHECKING([for exception-handling])
	AC_RUN_IFELSE([
		AC_LANG_PROGRAM(, [[
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
])


dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_REGEX([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl This function will test the existance of a POSIX compliant regex library.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_REGEX],
[
	AC_CHECK_HEADERS([sys/types.h])
	AC_MSG_CHECKING([for a POSIX compliant regex library])
	regex_found=no
	for REGEX_LIBS in '' -lgnurx -lregex; do
		MULE_BACKUP([LIBS])
		MULE_PREPEND([LIBS], [$REGEX_LIBS])
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#ifdef HAVE_SYS_TYPES_H
				#	include <sys/types.h>
				#endif
				#include <regex.h>
			]], [[
				regex_t preg;
				regcomp(&preg, "", REG_EXTENDED);
				regmatch_t *pmatch;
				regexec(&preg, "", 0, pmatch, 0);
				regfree(&preg);
			]])
		], [
			MULE_RESTORE([LIBS])
			regex_found=yes
			break;
		], [MULE_RESTORE([LIBS])])
	done
	AC_MSG_RESULT([$regex_found])
	ifelse([$1$2],,, [AS_IF([test $regex_found = yes], [$1], [$2])])
AC_SUBST([REGEX_LIBS])dnl
])


dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_CXXABI
dnl
dnl This function will test the header <cxxabi.h> and abi::__cxa_demangle()
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_CXXABI],
[AC_LANG_ASSERT([C++])dnl

	AC_MSG_CHECKING([for <cxxabi.h> and __cxa_demangle()])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#include <cxxabi.h>
		]], [[
			int status;
			char * demangled = abi::__cxa_demangle("", 0, 0, &status);
			std::type_info *ti = abi::__cxa_current_exception_type();
		]])
	], [
		AH_TEMPLATE([HAVE_CXXABI], [Define to 1 if you have the <cxxabi.h> header which declares abi::__cxa_demangle()])
		AC_DEFINE([HAVE_CXXABI])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])
])


dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_EXECINFO
dnl
dnl This function will test the header <execinfo.h> and backtrace()
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_EXECINFO],
[
	AC_MSG_CHECKING([for <execinfo.h> and backtrace()])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#include <execinfo.h>
		]], [[
			void *bt[1];
			int n = backtrace(&bt, 1);
			const char **bt_syms = backtrace_symbols(bt, n);
		]])
	], [
		AH_TEMPLATE([HAVE_EXECINFO], [Define to 1 if you have the <execinfo.h> header which declares backtrace()])
		AC_DEFINE([HAVE_EXECINFO])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])
])


dnl ---------------------------------------------------------------------------
dnl MULE_DENOISER
dnl
dnl Test for denoising level and add denoiser commands to config.status
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_DENOISER],
[
	AC_ARG_WITH([denoise-level],
		[AS_HELP_STRING([--with-denoise-level=<level>], [Specifies denoising level (0-4):])
		AS_HELP_STRING([], [0 - Do nothing])
		AS_HELP_STRING([], [4 - Suppress all normal output])
		AS_HELP_STRING([], [(for more information see src/utils/scripts/denoiser.rules)])
	])

	AC_MSG_CHECKING([denoising level])
	AS_IF([test ${with_denoise_level:-5} = yes], [with_denoise_level=5])
	AS_IF([test ${with_denoise_level:-5} = no], [with_denoise_level=0])
	AS_IF([test ${with_denoise_level:-5} -gt 4],
		[AS_IF([test "${svndate:+set}" = "set"], [with_denoise_level=0], [with_denoise_level=4])])
	AC_MSG_RESULT([$with_denoise_level])

	AC_CONFIG_COMMANDS([denoiser], [[if test $denoiserlevel -gt 0; then
		if test ! -d src/utils/scripts; then mkdir -p src/utils/scripts; fi
		sed -e "1{x;s/.*/1/;x;};/^[ 	]*\$/d;/^#if /{/level.*$denoiserlevel/{x;s/^/1/;x;b0;};x;s/^/0/;x;:0;d;};/^#else/{x;/^1/{s/1/0/;b1;};s/0/1/;:1;x;d;};/^#endif/{x;s/.//;x;d;};/^[ 	]*#/d;x;/^1/{x;b;};x;d" \
			$srcdir/src/utils/scripts/denoiser.rules > src/utils/scripts/denoiser.sed
		for i in `find . -name 'Makefile' -print`; do
			if test -n "`head -n 1 $i | grep '^#'`"; then
				sed -f src/utils/scripts/denoiser.sed $i > $i.tmp && mv $i.tmp $i
			fi
		done
	fi]], [denoiserlevel=$with_denoise_level])
])
