dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_LIBPNG_CONFIG
dnl
dnl adds support for --libpng-prefix and --libpng-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_LIBPNGCONFIG],
[
	AC_ARG_WITH(
		[libpng-prefix],
		[AS_HELP_STRING(
			[--with-libpng-prefix=PREFIX],
			[prefix where libpng is installed (optional)])],
		[libpng_config_prefix="$withval"],
		[libpng_config_prefix=""])
	AC_ARG_WITH(
		[libpng-config],
		[AS_HELP_STRING(
			[--with-libpng-config=CONFIG],
			[libpng-config script to use (optional)])],
		[libpng_config_name="$withval"],
		[libpng_config_name=""])
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_LIBPNGCONFIG([VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Test for libpng, and define LIBPNG*FLAGS, LIBPNG_LIBS and LIBPNG_CONFIG_NAME
dnl environment variable to override the default name of the libpng-config script
dnl to use. Set LIBPNG_CONFIG_PATH to specify the full path to libpng-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------
AC_DEFUN([AM_PATH_LIBPNGCONFIG],
[AC_REQUIRE([AM_OPTIONS_LIBPNGCONFIG])dnl
m4_define([REQUIRED_VERSION], [ifelse([$1],, [1.2.0], [$1])])dnl
m4_define([REQUIRED_VERSION_MAJOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\1])])dnl
m4_define([REQUIRED_VERSION_MINOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\2])])dnl
m4_define([REQUIRED_VERSION_MICRO], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\3])])dnl

  dnl do we have libpng-config name: it can be libpng-config or libpng12-config or ...
  if test x${LIBPNG_CONFIG_NAME+set} != xset ; then
     LIBPNG_CONFIG_NAME=libpng-config
  fi
  if test "x$libpng_config_name" != x ; then
     LIBPNG_CONFIG_NAME="$libpng_config_name"
  fi

  dnl deal with optional prefix
  if test x$libpng_config_prefix != x ; then
     LIBPNG_LOOKUP_PATH="$libpng_config_prefix/bin"
  fi
  
  dnl don't search the PATH if LIBPNG_CONFIG_NAME is absolute filename
  if test -x "$LIBPNG_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for libpng-config)
     LIBPNG_CONFIG_PATH="$LIBPNG_CONFIG_NAME"
     AC_MSG_RESULT($LIBPNG_CONFIG_PATH)
  else
     AC_PATH_PROG(LIBPNG_CONFIG_PATH, $LIBPNG_CONFIG_NAME, no, "$LIBPNG_LOOKUP_PATH:$PATH")
  fi

  if test "$LIBPNG_CONFIG_PATH" != "no" ; then
    LIBPNG_VERSION=""

    AC_MSG_CHECKING([for libpng version >= REQUIRED_VERSION])

    LIBPNG_CONFIG_WITH_ARGS="$LIBPNG_CONFIG_PATH $libpng_config_args"

    LIBPNG_VERSION=`$LIBPNG_CONFIG_WITH_ARGS --version`
    libpng_config_major_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\1/']`
    libpng_config_minor_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\2/']`
    libpng_config_micro_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\3/']`

    libpng_ver_ok=""
    if test $libpng_config_major_version -gt REQUIRED_VERSION_MAJOR; then
      libpng_ver_ok=yes
    else
      if test $libpng_config_major_version -eq REQUIRED_VERSION_MAJOR; then
         if test $libpng_config_minor_version -gt REQUIRED_VERSION_MINOR; then
            libpng_ver_ok=yes
         else
            if test $libpng_config_minor_version -eq REQUIRED_VERSION_MINOR; then
               if test $libpng_config_micro_version -ge REQUIRED_VERSION_MICRO; then
                  libpng_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test x$libpng_ver_ok = x ; then
      if test x$LIBPNG_VERSION = x; then
	dnl '$LIBPNG_CONFIG_WITH_ARGS --version' didn't produce output
	AC_MSG_RESULT([no])
      else
	AC_MSG_RESULT([no (version $LIBPNG_VERSION is not new enough)])
      fi
      LIBPNG_CFLAGS=""
      LIBPNG_CXXFLAGS=""
      LIBPNG_LDFLAGS=""
      LIBPNG_LIBS=""
      $3
    else
      LIBPNG_LIBS=`$LIBPNG_CONFIG_WITH_ARGS --libs`
      LIBPNG_LDFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --ldflags | sed -e "s/ *${LIBPNG_LIBS}$//"`
      LIBPNG_CFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --cflags`
      LIBPNG_CXXFLAGS=$LIBPNG_CFLAGS
      AC_MSG_RESULT([yes (version $LIBPNG_VERSION)])
      $2
    fi

  else
	dnl Some RedHat RPMs miss libpng-config, so test for
	dnl the usability with default options.
	AC_MSG_CHECKING([for libpng >= REQUIRED_VERSION])
	dnl Set up LIBS for the test
	saved_LIBS="$LIBS"
	LIBS="$LIBS -lpng -lz -lm"
	dnl Set default (empty) values.
	LIBPNG_CFLAGS=""
	LIBPNG_CXXFLAGS=""
	LIBPNG_LDFLAGS=""
	LIBPNG_LIBS=""
	AC_RUN_IFELSE([
		AC_LANG_PROGRAM([[
			#include <png.h>
			#include <stdio.h>

			/* Check linking to png library */
			void dummy() {
				png_check_sig(NULL, 0);
			}
		]], [dnl Don't use double-quoting here!
			/* png.h defines PNG_LIBPNG_VER=xyyzz */
			FILE *f=fopen("conftestval", "w");
			if (!f) exit(1);
			fprintf(f, "%s", (PNG_LIBPNG_VER >= REQUIRED_VERSION_MAJOR * 10000 + REQUIRED_VERSION_MINOR * 100 + REQUIRED_VERSION_MICRO) ? "yes" : "no");
			fclose(f);
			f=fopen("conftestver", "w");
			if (!f) exit(0);
			fprintf(f, "%s", PNG_LIBPNG_VER_STRING);
			fclose(f);
			exit(0);
		])
	], [
		AS_IF([test -f conftestval], [result=`cat conftestval`], [result=no])
		if test x$result = xyes; then
			if test -f conftestver; then
				LIBPNG_VERSION=`cat conftestver`
				lib_version=" (version $LIBPNG_VERSION)"
			else
				lib_version=""
			fi
		fi
		AC_MSG_RESULT([$result$lib_version])
		LIBPNG_LIBS="-lpng -lz -lm"
	], [
		result=no
		AC_MSG_RESULT([$result])
	], [
		AC_MSG_RESULT([cross-compilation detected, checking only the header])
		AC_CHECK_HEADER([png.h], [result=yes], [result=no])
		if test x$result = xyes; then
			LIBPNG_VERSION="detected"
			LIBPNG_LIBS="-lpng -lz -lm"
		fi
	])
	AS_IF([test x$result = xyes], [$2], [$3])
	dnl Restore LIBS
	LIBS="$saved_LIBS"
  fi

AC_SUBST(LIBPNG_CFLAGS)dnl
AC_SUBST(LIBPNG_CXXFLAGS)dnl
AC_SUBST(LIBPNG_LDFLAGS)dnl
AC_SUBST(LIBPNG_LIBS)dnl

m4_undefine([REQUIRED_VERSION])dnl
m4_undefine([REQUIRED_VERSION_MAJOR])dnl
m4_undefine([REQUIRED_VERSION_MINOR])dnl
m4_undefine([REQUIRED_VERSION_MICRO])dnl
])
