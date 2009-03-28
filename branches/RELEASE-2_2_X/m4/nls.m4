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

dnl ----------------------------------------------------
dnl MULE_CHECK_AUTOPOINT([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl check if autopoint is installed
dnl ----------------------------------------------------
AC_DEFUN([MULE_CHECK_AUTOPOINT],
[
	AC_MSG_CHECKING([for autopoint])
	autopoint_version=`autopoint --version | head -n 1 | sed -e 's/.*[[^0-9.]]\([[0-9]]\{1,\}\(\.[[0-9]]\{1,\}\)\{1,2\}\)[[^0-9.]]*/\1/'`
	AS_IF([test -n "$autopoint_version"], [
		AC_MSG_RESULT([yes ($autopoint_version)])
		$1
	], [
		AC_MSG_RESULT([no])
		$2
	])
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
	AS_IF([test "$[]$1" = "yes"], [
		AS_IF([test -z "$LINGUAS"],
			[$1_MANPAGES=`ls -1 ${srcdir}/$2.* | sed -e 's:.*/::g'`],
		[
			$1_MANPAGES=`ls -1 ${srcdir}/$2.* | sed -e 's:.*/::g' | grep $Generate_Langs `
			$1_MANPAGES="`basename $2.1` $[]$1_MANPAGES"
		])
		$1_MANPAGES=`echo $[]$1_MANPAGES | tr -d '\n'`
	], [$1_MANPAGES=])

AC_SUBST([$1_MANPAGES])dnl
])


dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_NLS
dnl
dnl Checks and tests everything needed for Native Language Support
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_NLS],
[
	AC_ARG_WITH([language],
		[AS_HELP_STRING([--with-language=<langs>],
			[Specify a comma-separated list of languages you want to have installed. See po/LINGUAS for available languages])],
		[AS_IF([test "$withval" = "all"], [LINGUAS='%UNSET%'], [LINGUAS="`echo $withval | sed -e 's/,/ /g'`"])])

	AM_GNU_GETTEXT([no-libtool], [need-ngettext])
	AS_IF([test $USE_INCLUDED_LIBINTL = yes], [INCINTL=-I\${top_builddir}/intl])

	AS_IF([test x$USE_NLS = xyes], [MULE_CHECK_AUTOPOINT(, [USE_NLS=no])])
	AS_IF([test x$USE_NLS = xno -a x${enable_nls:-yes} = xyes], [AC_MSG_WARN([You need to install GNU gettext/gettext-tools to compile aMule with i18n support.])])

	AS_IF([test ${USE_NLS:-no} = yes], [
		AC_MSG_CHECKING([for requested languages])
		Generate_Langs=`echo $LINGUAS | $AWK ['OFS="\\\\|" { for (i = 1; i <= NF; ++i) $i = "\\\\." $i; print }']`
		GENERATE_MANS_TO_INSTALL([AMULE_DAEMON], [docs/man/amuled])
		GENERATE_MANS_TO_INSTALL([AMULECMD], [docs/man/amulecmd])
		GENERATE_MANS_TO_INSTALL([WEB], [docs/man/amuleweb])
		GENERATE_MANS_TO_INSTALL([AMULE_GUI], [docs/man/amulegui])
		GENERATE_MANS_TO_INSTALL([CAS], [src/utils/cas/docs/cas])
		GENERATE_MANS_TO_INSTALL([WXCAS], [src/utils/wxCas/docs/wxcas])
		GENERATE_MANS_TO_INSTALL([ED2K], [docs/man/ed2k])
		GENERATE_MANS_TO_INSTALL([ALC], [src/utils/aLinkCreator/docs/alc])
		GENERATE_MANS_TO_INSTALL([ALCC], [src/utils/aLinkCreator/docs/alcc])
		GENERATE_MANS_TO_INSTALL([MONOLITHIC], [docs/man/amule])
		AC_MSG_RESULT([${LINGUAS:-all}])
	])

AC_SUBST([INCINTL])dnl
])
