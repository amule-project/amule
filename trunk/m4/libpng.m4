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
			[prefix where libpng is installed])],
		[libpng_config_prefix="$withval"],
		[libpng_config_prefix=""])
	AC_ARG_WITH(
		[libpng-exec-prefix],
		[AS_HELP_STRING(
			[--with-libpng-exec-prefix=PREFIX],
			[exec prefix where libpng  is installed])],
		[libpng_config_exec_prefix="$withval"],
		[libpng_config_exec_prefix=""])
	AC_ARG_WITH(
		[libpng-config],
		[AS_HELP_STRING(
			[--with-libpng-config=CONFIG],
			[libpng-config script to use])],
		[libpng_config_name="$withval"],
		[libpng_config_name=""])
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_LIBPNGCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for libpng, and define LIBPNG*FLAGS, LIBPNG_LIBS and LIBPNG_CONFIG_NAME
dnl environment variable to override the default name of the libpng-config script
dnl to use. Set LIBPNG_CONFIG_PATH to specify the full path to libpng-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the libpng-config script
dnl

AC_DEFUN([AM_PATH_LIBPNGCONFIG],
[
  dnl do we have libpng-config name: it can be libpng-config or gd-config or ...
  if test x${LIBPNG_CONFIG_NAME+set} != xset ; then
     LIBPNG_CONFIG_NAME=libpng-config
  fi
  if test "x$libpng_config_name" != x ; then
     LIBPNG_CONFIG_NAME="$libpng_config_name"
  fi

  dnl deal with optional prefixes
  if test x$libpng_config_exec_prefix != x ; then
     dnl libpng_config_args="$libpng_config_args --exec-prefix=$libpng_config_exec_prefix"
     LIBPNG_LOOKUP_PATH="$libpng_config_exec_prefix/bin"
  fi
  if test x$libpng_config_prefix != x ; then
     dnl libpng_config_args="$libpng_config_args --prefix=$libpng_config_prefix"
     LIBPNG_LOOKUP_PATH="$LIBPNG_LOOKUP_PATH:$libpng_config_prefix/bin"
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
    no_libpng=""

    min_libpng_version=ifelse([$1], ,1.2.0,$1)
    AC_MSG_CHECKING(for libpng version >= $min_libpng_version)

    LIBPNG_CONFIG_WITH_ARGS="$LIBPNG_CONFIG_PATH $libpng_config_args"

    LIBPNG_VERSION=`$LIBPNG_CONFIG_WITH_ARGS --version`
    libpng_config_major_version=`echo $LIBPNG_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\1/'`
    libpng_config_minor_version=`echo $LIBPNG_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\2/'`
    libpng_config_micro_version=`echo $LIBPNG_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\3/'`

    libpng_requested_major_version=`echo $min_libpng_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\1/'`
    libpng_requested_minor_version=`echo $min_libpng_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\2/'`
    libpng_requested_micro_version=`echo $min_libpng_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\3/'`

    libpng_ver_ok=""
    if test $libpng_config_major_version -gt $libpng_requested_major_version; then
      libpng_ver_ok=yes
    else
      if test $libpng_config_major_version -eq $libpng_requested_major_version; then
         if test $libpng_config_minor_version -gt $libpng_requested_minor_version; then
            libpng_ver_ok=yes
         else
            if test $libpng_config_minor_version -eq $libpng_requested_minor_version; then
               if test $libpng_config_micro_version -ge $libpng_requested_micro_version; then
                  libpng_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test "x$libpng_ver_ok" = x ; then
      no_libpng=yes
    else
      LIBPNG_LIBS=`$LIBPNG_CONFIG_WITH_ARGS --libs`
      LIBPNG_LDFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --ldflags`
      LIBPNG_CFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --cflags`
      LIBPNG_CXXFLAGS=$LIBPNG_CFLAGS
    fi

    if test "x$no_libpng" = x ; then
       AC_MSG_RESULT(yes (version $LIBPNG_VERSION))
       AC_CHECK_HEADER([gd.h],[$2],[$3])
    else
       if test "x$LIBPNG_VERSION" = x; then
	  dnl no libpng-config at all
	  AC_MSG_RESULT(no)
       else
	  AC_MSG_RESULT(no (version $LIBPNG_VERSION is not new enough))
       fi

       LIBPNG_CFLAGS=""
       LIBPNG_CXXFLAGS=""
       LIBPNG_LDFLAGS=""
       LIBPNG_LIBS=""
       ifelse([$3], , :, [$3])
    fi
  else
	dnl Some RedHat RPMs miss libpng-config, so test for
	dnl the usability with default options.
	dnl Checking for libpng >= 1.2.0, and ignoring passed VERSION ($1),
	dnl to make life simpler.
	AC_MSG_CHECKING([for libpng >= 1.2.0])
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
		]], [[
			/* png.h defines PNG_LIBPNG_VER=xyyzz */
			FILE *f=fopen("conftestval", "w");
			if (!f) exit(1);
			fprintf(f, "%s", (PNG_LIBPNG_VER >= 10200) ? "yes" : "no");
			fclose(f);
			f=fopen("conftestver", "w");
			if (!f) exit(0);
			fprintf(f, "%s", PNG_LIBPNG_VER_STRING);
			fclose(f);
			exit(0);
		]])
	], [
		if test -f conftestval; then
	        	result=`cat conftestval`
		else
			result=no
		fi
		if test x$result = xyes; then
			if test -f conftestver; then
				LIBPNG_VERSION=`cat conftestver`
				lib_version=" (version $LIBPNG_VERSION)"
			else
				lib_version=""
			fi
		fi
		AC_MSG_RESULT($result$lib_version)
		LIBPNG_LIBS="-lpng -lz -lm"
	], [
		result=no
		AC_MSG_RESULT($result)
	], [
		AC_MSG_RESULT([cross-compilation detected, checking only the header])
		AC_CHECK_HEADER(png.h, [result=yes], [result=no])
		if test x$result = xyes; then
			LIBPNG_VERSION="detected"
			LIBPNG_LIBS="-lpng -lz -lm"
		fi
	])
	if test x$result = xyes; then
		ifelse([$2], , :, [$2])
	else
		ifelse([$3], , :, [$3])
	fi
	dnl Restore LIBS
	LIBS="$saved_LIBS"
  fi


  AC_SUBST(LIBPNG_CFLAGS)
  AC_SUBST(LIBPNG_CXXFLAGS)
  AC_SUBST(LIBPNG_LDFLAGS)
  AC_SUBST(LIBPNG_LIBS)
])
