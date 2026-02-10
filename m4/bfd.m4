#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2015 aMule Team ( admin@amule.org / http://www.amule.org )
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
dnl MULE_CHECK_BFD
dnl
dnl Check if bfd.h is on the system and usable. Also checks whether we can link
dnl to the bdf functions and which libraries are needed for this. The result of
dnl the library test is saved in the mule_cv_lib_bfd cache variable, to ensure
dnl that further configurations can reuse this result. In the worst case it may
dnl take up to 32 link tests to find (or decide that it cannot be found) the
dnl right set of libraries.
dnl
dnl Compilation variables set by this function:
dnl	BFD_CPPFLAGS
dnl	BFD_LIBS
dnl
dnl Cache variables used/set:
dnl	ac_cv_header_ansidecl_h
dnl	ac_cv_header_bfd_h
dnl	mule_cv_lib_bfd
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_BFD],
[AC_REQUIRE([MULE_CHECK_NLS])dnl
AC_REQUIRE([AC_PROG_AWK])dnl

	AC_CHECK_HEADERS([ansidecl.h bfd.h])
	AS_IF([test $ac_cv_header_ansidecl_h = yes -a $ac_cv_header_bfd_h = yes],
	[
		AC_MSG_CHECKING([for libraries required to link with bfd])
		AC_CACHE_VAL([mule_cv_lib_bfd],
		[
			for bfd_ldadd in MULE_COMBINATE([-lbfd], [-liberty], [-ldl], [${LIBINTL}], [${ZLIB_LIBS}]); do
				# Doing some black magic to prevent multiple tests for the same set of
				# libraries when any of the shell variables above expand to nothing.
				echo $bfd_ldadd
			done | sed -e 's/^ *//;s/ *$//;s/  */ /g' | ${AWK} '!x@<:@$[]0@:>@++' >conftest.bfd_ldadd
			while read bfd_ldadd; do
				MULE_BACKUP([LIBS])
				MULE_BACKUP([LDFLAGS])
				MULE_PREPEND([LIBS], [${bfd_ldadd}])
				MULE_APPEND([LDFLAGS], [${ZLIB_LDFLAGS}])
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <ansidecl.h>
						#include <bfd.h>
					]], [[const char *dummy = bfd_errmsg(bfd_get_error());]])
				], [
					MULE_RESTORE([LIBS])
					MULE_RESTORE([LDFLAGS])
					mule_cv_lib_bfd="${bfd_ldadd}"
				])
				MULE_RESTORE([LIBS])
				MULE_RESTORE([LDFLAGS])
				AS_IF([${mule_cv_lib_bfd+:} false], [break])
			done <conftest.bfd_ldadd
			rm -f conftest.bfd_ldadd
		])

		AS_IF([${mule_cv_lib_bfd+:} false],
		[
			BFD_CPPFLAGS="-DHAVE_BFD"
			BFD_LIBS="${mule_cv_lib_bfd}"
			AC_MSG_RESULT([${BFD_LIBS:-none required}])
		], [
			AC_MSG_RESULT([not found])
			MULE_WARNING([Cannot link to the library containing the bfd functions.])
		])
	], [
		MULE_WARNING([bfd.h not found or unusable, please install binutils development package if you are a developer or want to help testing aMule])
	])

AC_SUBST([BFD_CPPFLAGS])dnl
AC_SUBST([BFD_LIBS])dnl
])
