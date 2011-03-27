#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

m4_pattern_forbid(MULE_)dnl Check for unexpanded *MULE_* macros
m4_pattern_allow(AMULE_)dnl Allow the *AMULE_* names
m4_pattern_forbid(__mule_)dnl Check for unexpanded internal macros


# -------------------- #
# Common useful macros #
# -------------------- #

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


# ------------------- #
# Issuing diagnostics #
# ------------------- #

# -----------------------------------------------------------------------------
# __mule_print_final_warning(section, condition, message)
# -----------------------------------------------------------------------------
m4_define([__mule_print_final_warning],
[m4_divert_push($1)dnl
if test [$2]; then
cat <<_MULEEOT

m4_pushdef([__mule_Prefix1], [* ])dnl
m4_pushdef([__mule_Prefix], [  ])dnl
m4_foreach([__mule_Line], m4_quote(m4_split([$3], [
])), [m4_text_wrap(m4_defn([__mule_Line]), __mule_Prefix, __mule_Prefix1)
m4_define([__mule_Prefix1], __mule_Prefix)dnl
])[]dnl
m4_popdef([__mule_Prefix])dnl
m4_popdef([__mule_Prefix1])dnl
_MULEEOT
fi
m4_divert_pop()])

dnl ---------------------------------------------------------------------------
dnl MULE_WARNING(MESSAGE)
dnl
dnl Works like AC_MSG_WARN(), but the warning will be reproduced at the end of
dnl the configure run. An empty line is prepended at the final output and a
dnl newline is appended for free.
dnl ---------------------------------------------------------------------------
m4_ifndef([_MULE_WARNINGS],[
m4_define([_m4_divert(_MULE_WARNINGS)], m4_incr(_m4_divert([BODY])))])
m4_define([_MULE_WARNCOUNT], [0])

m4_divert_push(_MULE_WARNINGS)dnl
if test ${_mule_has_warnings:-no} = yes; then
echo ""
echo ""
echo " *** Warnings during configuration ***"
fi
m4_divert_pop()dnl

m4_define([MULE_WARNING],
[AC_MSG_WARN(
m4_pushdef([__mule_Prefix], [        ])dnl
m4_foreach([__mule_Line], m4_quote(m4_split([$1], [
])), [
m4_text_wrap(m4_defn([__mule_Line]), __mule_Prefix)])[]dnl
m4_popdef([__mule_Prefix])dnl
)
_mule_warning_[]_MULE_WARNCOUNT[]=yes
_mule_has_warnings=yes
__mule_print_final_warning([_MULE_WARNINGS], [${_mule_warning_]_MULE_WARNCOUNT[:-no} = yes], [$1])dnl
m4_define([_MULE_WARNCOUNT], incr(_MULE_WARNCOUNT))])

dnl ---------------------------------------------------------------------------
dnl MULE_DEPRECATED(OLDFLAG [, NEWFLAG])
dnl
dnl Marks OLDFLAG as deprecated and produces an appropriate warning. If NEWFLAG
dnl is specified and is unset the value of OLDFLAG is assigned to NEWFLAG (i.e.
dnl if the user specified both OLDFLAG and NEWFLAG, NEWFLAG takes precedence;
dnl if only OLDFLAG is specified it will be redirected to NEWFLAG).
dnl
dnl There should be no AC_ARG_* for the deprecated flag, and if the old flag is
dnl deprecated in favour of a new one, MULE_DEPRECATED *MUST* precede the
dnl AC_ARG_* definition of the new flag (otherwise redirection may not work).
dnl ---------------------------------------------------------------------------
m4_define([_MULE_DEPRECATIONWARNINGS], [incr(_MULE_WARNINGS)])
m4_define([__mule_display_option_name], [m4_if(m4_substr([$1], 0, 1), [-],, [--])m4_bpatsubst([$1], [_], [-])])
m4_define([__mule_ac_option_name], [m4_bpatsubst(m4_bpatsubst(m4_bpatsubst(m4_bpatsubst([$1], [^-+], []), [-], [_]), [^disable], [enable]), [^without], [with])])

m4_define([MULE_DEPRECATED],
[if test "${__mule_ac_option_name([$1])+set}" = "set"; then
  _mule_has_warnings=yes
m4_ifvaln([$2], [  if test "${__mule_ac_option_name([$2]):-unset}" = "unset"; then
    __mule_ac_option_name([$2])=$__mule_ac_option_name([$1])
  fi])fi
__mule_print_final_warning([_MULE_DEPRECATIONWARNINGS], ["${]__mule_ac_option_name([$1])[+set}" = set], __mule_display_option_name([$1])[ is now deprecated and ]m4_ifval([$2], [might be removed in the future without further notice. Please use ]__mule_display_option_name([$2])[ instead.], [not supported anymore.]))])


# ----------------- #
# Argument handling #
# ----------------- #
m4_define([__mule_arg_default], [__mule_arg_[]m4_translit([$1], [-], [_])[]_default])
m4_define([__mule_arg_value], [${enable_[]m4_translit([$1], [-], [_])[]:-[]__mule_arg_default([$1])[]}])

dnl ---------------------------------------------------------------------------
dnl MULE_ARG_ENABLE(FEATURE, DEFAULT-VALUE, HELP-STRING [, AUTOMAKE-CONDITIONAL])
dnl
dnl Wrapper around AC_ARG_ENABLE() that supports automatically setting up a
dnl conditional variable for automake, remembering default value for
dnl conditionals and supplying the help string based on the default value (i.e.
dnl it produce "--enable-FEATURE   HELP-STRING" if the default is no, and
dnl "--disable-FEATURE    HELP-STRING" if the default is yes. The default value
dnl *MUST* be either `yes' or `no'.
dnl ---------------------------------------------------------------------------
m4_define([MULE_ARG_ENABLE],
[m4_if([$2], [yes],, [m4_if([$2], [no],, [m4_fatal([Default value must be either `yes' or `no'!])])])dnl
m4_define(__mule_arg_default([$1]), [$2])dnl
AC_ARG_ENABLE([$1], [AS_HELP_STRING(m4_if([$2], [yes], [--disable-$1], [--enable-$1]), [$3])])
m4_ifvaln([$4], [AM_CONDITIONAL([$4], [test ]__mule_arg_value([$1])[ = yes])])])

dnl ---------------------------------------------------------------------------
dnl MULE_IS_ENABLED(FEATURE)
dnl
dnl Used in shell conditionals, tests whether the named feature is enabled or
dnl not, considering also the default value. FEATURE *must* have been set up
dnl using MULE_ARG_ENABLE().
dnl ---------------------------------------------------------------------------
m4_define([MULE_IS_ENABLED],
[m4_ifdef(__mule_arg_default([$1]), __mule_arg_value([$1])[ = yes], [m4_fatal([Unknown feature `$1'!])])])

m4_define([MULE_IS_ENABLED_ANY], [__mule_if_multi([$1], [-o])])
m4_define([MULE_IS_ENABLED_ALL], [__mule_if_multi([$1], [-a])])

dnl ---------------------------------------------------------------------------
dnl MULE_ENABLEVAR(FEATURE)
dnl
dnl Expands to the name of the shell variable holding the enabled/disabled
dnl status of FEATURE. FEATRUE *must* have been set up using MULE_ARG_ENABLE().
dnl ---------------------------------------------------------------------------
m4_define([MULE_ENABLEVAR],
[m4_ifdef(__mule_arg_default([$1]), [enable_[]m4_translit([$1], [-], [_])], [m4_fatal([Unknown feature `$1'!])])])

dnl ---------------------------------------------------------------------------
dnl MULE_STATUSOF(FEATURE)
dnl
dnl Expands to the value of the shell variable holding the status of FEATURE,
dnl considering default values. FEATURE *must* have been set up using
dnl MULE_ARG_ENABLE().
dnl ---------------------------------------------------------------------------
m4_define([MULE_STATUSOF],
[m4_ifdef(__mule_arg_default([$1]), __mule_arg_value([$1]), [m4_fatal([Unknown feature `$1'!])])])


# ---------------------- #
# Conditional processing #
# ---------------------- #
m4_define([__mule_if_multi],
[m4_define([__mule_if_logic], [])dnl
m4_foreach([__mule_condition], [$1], [__mule_if_logic MULE_IS_ENABLED(__mule_condition) ][m4_define([__mule_if_logic], [$2])])dnl
m4_undefine([__mule_if_logic])])

dnl ---------------------------------------------------------------------------
dnl MULE_IF_ENABLED(FEATURE, [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl Basically a wrapper around AS_IF(), the test being if FEATURE is enabled.
dnl FEATURE must have been set up by MULE_ARG_ENABLE().
dnl ---------------------------------------------------------------------------
m4_define([MULE_IF_ENABLED],
[AS_IF([test MULE_IS_ENABLED([$1])], [$2], [$3])])

m4_define([MULE_IF_ENABLED_ALL],
[AS_IF([test]__mule_if_multi([$1], [-a]), [$2], [$3])])

m4_define([MULE_IF_ENABLED_ANY],
[AS_IF([test]__mule_if_multi([$1], [-o]), [$2], [$3])])

dnl ---------------------------------------------------------------------------
dnl MULE_IF(CONDITION, [IF-TRUE] [, ELIF-CONDITION, [IF-TRUE]]... [, ELSE-BRANCH])
dnl
dnl Works like AS_IF(), but allows elif-branches too.
dnl ---------------------------------------------------------------------------
m4_define([__mule_if_helper],
[m4_if( [$#], 0,,
	[$#], 1, [m4_ifvaln([$1], [m4_n([else])  $1])],
	[m4_n([elif $1; then])  m4_ifvaln([$2], [$2], :)])dnl
m4_if(m4_eval([$# > 2]), 1, [$0(m4_shiftn(2, $@))])])

m4_define([MULE_IF],
[m4_if( [$#], 0,,
	[$#], 1,,
	[m4_n([if $1; then])  m4_ifval([$2],[$2], :)
m4_if(m4_eval([$# > 2]), 1, [__mule_if_helper(m4_shiftn(2, $@))])m4_n([fi])])])


# ----------------------- #
# Other high-level macros #
# ----------------------- #

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
		MULECPPFLAGS="-D__OPENBSD__"
		;;
	*cygwin* | *mingw32*)
		SYS=win32
		MULECPPFLAGS="-DNOMINMAX"
		;;
	solaris*)
		SYS=solaris
		RESOLV_LIB="-lresolv -lnsl"
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
AC_SUBST([MULECPPFLAGS])dnl
AC_SUBST([MULECFLAGS])dnl
AC_SUBST([MULECXXFLAGS])dnl
AC_SUBST([MULELDFLAGS])dnl
AC_SUBST([MULERCFLAGS])dnl
])

dnl ---------------------------------------------------------------------------
dnl MULE_COMPILATION_FLAGS
dnl
dnl Checks type of compilation requested by user, and sets various flags
dnl accordingly.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_COMPILATION_FLAGS],
[AC_REQUIRE([MULE_CHECK_GLIBCXX])dnl

	MULE_ARG_ENABLE([debug],	[yes],	[disable additional debugging output])
	MULE_ARG_ENABLE([profile],	[no],	[enable code profiling])
	MULE_ARG_ENABLE([optimize],	[no],	[enable code optimization])

	MULE_IF_ENABLED([debug],
	[
		MULE_ADDFLAG([CPP], [-D__DEBUG__])
		MULE_ADDCCXXFLAG([-g])
		AS_IF([test ${GLIBCXX:-no} = yes],	[MULE_ADDFLAG([CPP], [-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC])])
		AS_IF([test ${GCC:-no} = yes],		[MULE_ADDCCXXFLAG([-W -Wall -Wshadow -Wundef -ggdb -fno-inline -fmessage-length=0])])
		AS_IF([test ${SYS:-unknown} = win32],	[MULE_ADDFLAG([RC], [-D__DEBUG__])])
	], [
		AS_IF([test ${GCC:-no} = yes], [MULE_ADDCCXXFLAG([-W -Wall -Wshadow -Wundef])])
	])

	MULE_IF_ENABLED([profile],
	[
		MULE_ADDCCXXFLAG([-pg])
		MULE_ADDFLAG([LD], [-pg])
	])

	MULE_IF_ENABLED([optimize],	[MULE_ADDCCXXFLAG([-O2])])

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
			#include <wx/filefn.h>
			#ifndef wxHAS_LARGE_FILES
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
	MULE_ARG_ENABLE([ccache], [no], [enable ccache support for fast recompilation])

	AC_ARG_WITH([ccache-prefix],
		[AS_HELP_STRING([--with-ccache-prefix=PREFIX], [prefix where ccache is installed])])

	AC_MSG_CHECKING([whether ccache support should be added])
	AC_MSG_RESULT([MULE_STATUSOF([ccache])])

	MULE_IF_ENABLED([ccache], [
		AC_MSG_CHECKING([for ccache presence])
		AS_IF([test -z "$with_ccache_prefix"], [
			ccache_full=`which ccache`
			with_ccache_prefix=`dirname ${ccache_full}`
		])
		AS_IF([$with_ccache_prefix/ccache -V >/dev/null 2>&1], [
			CC="$with_ccache_prefix/ccache $CC"
			CXX="$with_ccache_prefix/ccache $CXX"
			BUILD_CC="$with_ccache_prefix/ccache $BUILD_CC"
		], [MULE_ENABLEVAR([ccache])=no])
		AC_MSG_RESULT([MULE_STATUSOF([ccache])])
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
	for bfd_ldadd in "" "${LIBINTL}" "-ldl" "-ldl ${LIBINTL}"; do
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
		[MULE_WARNING([bfd.h not found or unusable, please install binutils development package if you are a developer or want to help testing aMule])])

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
		MULE_WARNING(
			[Cross-compilation detected, so exception handling cannot be tested.
			Note that broken exception handling in your compiler may lead to unexpected crashes.])
	])
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
dnl MULE_CHECK_MMAP
dnl
dnl Checks for mmap() and makes use of it when found.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_MMAP],
[
	MULE_ARG_ENABLE([mmap], [no], [enable using mapped memory if supported])

	MULE_IF_ENABLED([mmap], [
		AC_CHECK_HEADERS([sys/mman.h])
		AC_FUNC_MMAP
		AC_CHECK_FUNCS([munmap sysconf])
		AS_IF([test $ac_cv_func_sysconf = yes], [
			AC_MSG_CHECKING([for pagesize constant for sysconf])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#include <unistd.h>
				]], [[
					return sysconf(_SC_PAGESIZE);
				]])
			], [
				AC_MSG_RESULT([_SC_PAGESIZE])
				AC_DEFINE([HAVE__SC_PAGESIZE], [1], [Define to 1 if you have the _SC_PAGESIZE constant in <unistd.h>])
			], [
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <unistd.h>
					]], [[
						return sysconf(_SC_PAGE_SIZE);
					]])
				], [
					AC_MSG_RESULT([_SC_PAGE_SIZE])
					AC_DEFINE([HAVE__SC_PAGE_SIZE], [1], [Define to 1 if you have the _SC_PAGE_SIZE constant in <unistd.h>, but not _SC_PAGESIZE])
				], [
					AC_MSG_RESULT([none])
				])
			])
		])
	], [
		# fake the result of the test for munmap() for the gettext macros
		ac_cv_func_munmap=no
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
