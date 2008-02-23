dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_GDLIBCONFIG
dnl
dnl adds support for --gdlib-prefix and --gdlib-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_GDLIBCONFIG],
[
	AC_ARG_WITH(
		[gdlib-prefix],
		[AS_HELP_STRING(
			[--with-gdlib-prefix=PREFIX],
			[prefix where gdlib is installed (optional)])],
		[gdlib_config_prefix="$withval"],
		[gdlib_config_prefix=""])
	AC_ARG_WITH(
		[gdlib-config],
		[AS_HELP_STRING(
			[--with-gdlib-config=CONFIG],
			[gdlib-config script to use (optional)])],
		[gdlib_config_name="$withval"],
		[gdlib_config_name=""])
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_GDLIBCONFIG([VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Test for gdlib, and define GDLIB*FLAGS, GDLIB_LIBS and GDLIB_CONFIG_NAME
dnl environment variable to override the default name of the gdlib-config script
dnl to use. Set GDLIB_CONFIG_PATH to specify the full path to gdlib-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------
AC_DEFUN([AM_PATH_GDLIBCONFIG],
[AC_REQUIRE([AM_OPTIONS_GDLIBCONFIG])dnl
m4_define([REQUIRED_VERSION], [ifelse([$1],, [2.0.0], [$1])])dnl
m4_define([REQUIRED_VERSION_MAJOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\1])])dnl
m4_define([REQUIRED_VERSION_MINOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\2])])dnl
m4_define([REQUIRED_VERSION_MICRO], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\3])])dnl

  dnl do we have gdlib-config name: it can be gdlib-config or gd-config or ...
  if test x${GDLIB_CONFIG_NAME+set} != xset ; then
     GDLIB_CONFIG_NAME=gdlib-config
  fi
  if test "x$gdlib_config_name" != x ; then
     GDLIB_CONFIG_NAME="$gdlib_config_name"
  fi

  dnl deal with optional prefix
  if test x$gdlib_config_prefix != x ; then
     GDLIB_LOOKUP_PATH="$gdlib_config_prefix/bin"
  fi
  
  dnl don't search the PATH if GDLIB_CONFIG_NAME is absolute filename
  if test -x "$GDLIB_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for gdlib-config)
     GDLIB_CONFIG_PATH="$GDLIB_CONFIG_NAME"
     AC_MSG_RESULT($GDLIB_CONFIG_PATH)
  else
     AC_PATH_PROG(GDLIB_CONFIG_PATH, $GDLIB_CONFIG_NAME, no, "$GDLIB_LOOKUP_PATH:$PATH")
  fi

  if test "$GDLIB_CONFIG_PATH" != "no" ; then
    GDLIB_VERSION=""

    AC_MSG_CHECKING([for gdlib version >= REQUIRED_VERSION])

    GDLIB_CONFIG_WITH_ARGS="$GDLIB_CONFIG_PATH $gdlib_config_args"

    GDLIB_VERSION=`$GDLIB_CONFIG_WITH_ARGS --version`
    gdlib_config_major_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1/']`
    gdlib_config_minor_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\2/']`
    gdlib_config_micro_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\3/']`

    gdlib_ver_ok=""
    if test $gdlib_config_major_version -gt REQUIRED_VERSION_MAJOR; then
      gdlib_ver_ok=yes
    else
      if test $gdlib_config_major_version -eq REQUIRED_VERSION_MAJOR; then
         if test $gdlib_config_minor_version -gt REQUIRED_VERSION_MINOR; then
            gdlib_ver_ok=yes
         else
            if test $gdlib_config_minor_version -eq REQUIRED_VERSION_MINOR; then
               if test $gdlib_config_micro_version -ge REQUIRED_VERSION_MICRO; then
                  gdlib_ver_ok=yes
               fi
            fi
         fi
      fi
    fi
  fi

  if test x$gdlib_ver_ok = x ; then
    if test x$GDLIB_VERSION = x; then
      dnl no gdlib-config at all
      AC_MSG_RESULT([no])
    else
      AC_MSG_RESULT([no (version $GDLIB_VERSION is not new enough)])
    fi
    GDLIB_CFLAGS=""
    GDLIB_LDFLAGS=""
    GDLIB_LIBS=""
    $3
  else
    AC_MSG_RESULT([yes (version $GDLIB_VERSION)])
    GDLIB_CFLAGS=`$GDLIB_CONFIG_WITH_ARGS --cflags`
    GDLIB_LDFLAGS=`$GDLIB_CONFIG_WITH_ARGS --ldflags`
    GDLIB_LIBS=`$GDLIB_CONFIG_WITH_ARGS --libs`
    AC_CHECK_HEADER([gd.h],[$2],[$3])
  fi

AC_SUBST(GDLIB_CFLAGS)dnl
AC_SUBST(GDLIB_LDFLAGS)dnl
AC_SUBST(GDLIB_LIBS)dnl

m4_undefine([REQUIRED_VERSION])dnl
m4_undefine([REQUIRED_VERSION_MAJOR])dnl
m4_undefine([REQUIRED_VERSION_MINOR])dnl
m4_undefine([REQUIRED_VERSION_MICRO])dnl
])
