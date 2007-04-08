dnl ----------------------------------------------------
dnl CHECK_WX_BUILT_WITH_GTK2
dnl check gtk version wx widgets was compiled
dnl ----------------------------------------------------

AC_DEFUN([CHECK_WX_BUILT_WITH_GTK2],
[
  AC_MSG_CHECKING(if wxWidgets was linked with GTK2)
  if $WX_CONFIG_WITH_ARGS --cppflags | grep -q 'gtk2' ; then
     GTK_USEDVERSION=2
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

  AC_SUBST(GTK_USEDVERSION)
])

dnl ----------------------------------------------------
dnl GET_WX_VERSION
dnl get wx widgets
dnl ----------------------------------------------------

AC_DEFUN([GET_WX_VERSION],
[
  WX_VERSION_FULL=`$WX_CONFIG_WITH_ARGS --version`
  WX_VERSION_MAJOR=`echo $WX_VERSION_FULL | cut -d . -f 1`
  WX_VERSION_MINOR=`echo $WX_VERSION_FULL | cut -d . -f 2`
  WX_VERSION_RELEASE=`echo $WX_VERSION_FULL | cut -d . -f 3`
])


dnl ----------------------------------------------------
dnl GET_GTK_VERSION
dnl get gtk 1.x version
dnl ----------------------------------------------------

AC_DEFUN([GET_GTK_VERSION],
[
  GTK_VERSION=`$GTK_CONFIG --version`
  AC_SUBST(GTK_VERSION)
])

dnl ----------------------------------------------------
dnl GET_GTK2_VERSION
dnl get gtk 2.x version
dnl ----------------------------------------------------

AC_DEFUN([GET_GTK2_VERSION],
[
  GTK_VERSION=`$PKG_CONFIG --modversion gtk+-2.0`
  AC_SUBST(GTK_VERSION)
])

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

dnl END_OF_PNG

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
			[prefix where gdlib is installed])],
		[gdlib_config_prefix="$withval"],
		[gdlib_config_prefix=""])
	AC_ARG_WITH(
		[gdlib-exec-prefix],
		[AS_HELP_STRING(
			[--with-gdlib-exec-prefix=PREFIX],
			[exec prefix where gdlib  is installed])],
		[gdlib_config_exec_prefix="$withval"],
		[gdlib_config_exec_prefix=""])
	AC_ARG_WITH(
		[gdlib-config],
		[AS_HELP_STRING(
			[--with-gdlib-config=CONFIG],
			[gdlib-config script to use])],
		[gdlib_config_name="$withval"],
		[gdlib_config_name=""])
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_GDLIBCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for gdlib, and define GDLIB*FLAGS, GDLIB_LIBS and GDLIB_CONFIG_NAME
dnl environment variable to override the default name of the gdlib-config script
dnl to use. Set GDLIB_CONFIG_PATH to specify the full path to gdlib-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the gdlib-config script
dnl
AC_DEFUN([AM_PATH_GDLIBCONFIG],
[
  dnl do we have gdlib-config name: it can be gdlib-config or gd-config or ...
  if test x${GDLIB_CONFIG_NAME+set} != xset ; then
     GDLIB_CONFIG_NAME=gdlib-config
  fi
  if test "x$gdlib_config_name" != x ; then
     gdlib_CONFIG_NAME="$gdlib_config_name"
  fi

  dnl deal with optional prefixes
  if test x$gdlib_config_exec_prefix != x ; then
     dnl gdlib-config doesn't accept --exec-prefix
     dnl gdlib_config_args="$gdlib_config_args --exec-prefix=$gdlib_config_exec_prefix"
     GDLIB_LOOKUP_PATH="$gdlib_config_exec_prefix/bin"
  fi
  if test x$gdlib_config_prefix != x ; then
     dnl gdlib-config doesn't accept --prefix
     dnl gdlib_config_args="$gdlib_config_args --prefix=$gdlib_config_prefix"
     GDLIB_LOOKUP_PATH="$GDLIB_LOOKUP_PATH:$gdlib_config_prefix/bin"
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
    no_gdlib=""

    min_gdlib_version=ifelse([$1], ,2.0.0,$1)
    AC_MSG_CHECKING(for gdlib version >= $min_gdlib_version)

    GDLIB_CONFIG_WITH_ARGS="$GDLIB_CONFIG_PATH $gdlib_config_args"

    GDLIB_VERSION=`$GDLIB_CONFIG_WITH_ARGS --version`
    gdlib_config_major_version=`echo $GDLIB_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\1/'`
    gdlib_config_minor_version=`echo $GDLIB_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\2/'`
    gdlib_config_micro_version=`echo $GDLIB_VERSION | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\3/'`

    gdlib_requested_major_version=`echo $min_gdlib_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\1/'`
    gdlib_requested_minor_version=`echo $min_gdlib_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\2/'`
    gdlib_requested_micro_version=`echo $min_gdlib_version | \
        sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)\([[0-9|a-z]]*\)/\3/'`

    gdlib_ver_ok=""
    if test $gdlib_config_major_version -gt $gdlib_requested_major_version; then
      gdlib_ver_ok=yes
    else
      if test $gdlib_config_major_version -eq $gdlib_requested_major_version; then
         if test $gdlib_config_minor_version -gt $gdlib_requested_minor_version; then
            gdlib_ver_ok=yes
         else
            if test $gdlib_config_minor_version -eq $gdlib_requested_minor_version; then
               if test $gdlib_config_micro_version -ge $gdlib_requested_micro_version; then
                  gdlib_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test "x$gdlib_ver_ok" = x ; then
      no_gdlib=yes
    else
      GDLIB_LIBS=`$GDLIB_CONFIG_WITH_ARGS --libs`

      if test "x$gdlib_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         GDLIB_CFLAGS=`$GDLIB_CONFIG_WITH_ARGS --cflags`
         GDLIB_CPPFLAGS=$GDLIB_CFLAGS
         GDLIB_CXXFLAGS=$GDLIB_CFLAGS
	 GDLIB_LDFLAGS=`$GDLIB_CONFIG_WITH_ARGS --ldflags`
         GDLIB_CFLAGS_ONLY=$GDLIB_CFLAGS
         GDLIB_CXXFLAGS_ONLY=$GDLIB_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS -- ??
         GDLIB_CPPFLAGS=`$GDLIB_CONFIG_WITH_ARGS --cflags`
         GDLIB_CXXFLAGS=`$GDlIB_CONFIG_WITH_ARGS --cflags`
         GDLIB_CFLAGS=`$GDLIB_CONFIG_WITH_ARGS --cflags`
	 GDLIB_LDFLAGS=`$GDLIB_CONFIG_WITH_ARGS --ldflags`

         GDLIB_CFLAGS_ONLY=`echo $GDLIB_CFLAGS | sed "s@^$GDLIB_CPPFLAGS *@@"`
         GDLIB_CXXFLAGS_ONLY=`echo $GDLIB_CXXFLAGS | sed "s@^$GDLIB_CFLAGS *@@"`
      fi
    fi

    if test "x$no_gdlib" = x ; then
       AC_MSG_RESULT(yes (version $GDLIB_VERSION))
       AC_CHECK_HEADER([gd.h],[$2],[$3])
    else
       if test "x$GDLIB_VERSION" = x; then
	  dnl no gdlib-config at all
	  AC_MSG_RESULT(no)
       else
	  AC_MSG_RESULT(no (version $GDLIB_VERSION is not new enough))
       fi

       GDLIB_CFLAGS=""
       GDLIB_CPPFLAGS=""
       GDLIB_CXXFLAGS=""
       GDLIB_LDFLAGS=""
       GDLIB_LIBS=""
       GDLIB_LIBS_STATIC=""
       ifelse([$3], , :, [$3])
    fi
  fi


  AC_SUBST(GDLIB_CPPFLAGS)
  AC_SUBST(GDLIB_CFLAGS)
  AC_SUBST(GDLIB_CXXFLAGS)
  AC_SUBST(GDLIB_LDFLAGS)
  AC_SUBST(GDLIB_CFLAGS_ONLY)
  AC_SUBST(GDLIB_CXXFLAGS_ONLY)
  AC_SUBST(GDLIB_LIBS)
  AC_SUBST(GDLIB_LIBS_STATIC)
  AC_SUBST(GDLIB_VERSION)
])

dnl --------------------------------------------------------------------------
dnl Check for crypto++ library
dnl --------------------------------------------------------------------------
 

AC_DEFUN([CHECK_CRYPTO], [

CRYPTO_PP_STYLE="unknown"

if test x$USE_EMBEDDED_CRYPTO = xno; then

	  min_crypto_version=ifelse([$1], ,5.1,$1)
	  crypto_version=0;
	  
	  AC_MSG_CHECKING([for crypto++ version >= $min_crypto_version])
	  
	  # We don't use AC_CHECK_FILE to avoid caching.

	  if test x$crypto_prefix != x ; then
		  if test -f $crypto_prefix/cryptopp/cryptlib.h; then
		  CRYPTO_PP_STYLE="sources"
		  crypto_version=`grep "Reference Manual" $crypto_prefix/cryptopp/cryptlib.h | sed -e's#.*\s\(\([0-9]\+\.\?\)\+\)\s.*#\1#g'`
		  fi
	  else
          crypto_prefix="/usr"
          fi
          
	  if test -f $crypto_prefix/include/cryptopp/cryptlib.h; then
		  CRYPTO_PP_STYLE="mdk_suse_fc"
		  crypto_version=`grep "Reference Manual" $crypto_prefix/include/cryptopp/cryptlib.h | sed -e's#.*\s\(\([0-9]\+\.\?\)\+\)\s.*#\1#g'`
	  fi
	  
	  if test -f $crypto_prefix/include/crypto++/cryptlib.h; then
		  CRYPTO_PP_STYLE="gentoo_debian"
		  crypto_version=`grep "Reference Manual" $crypto_prefix/include/crypto++/cryptlib.h | sed -e's#.*\s\(\([0-9]\+\.\?\)\+\)\s.*#\1#g'`
	  fi

	  vers=`echo $crypto_version | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
	  minvers=`echo $min_crypto_version | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
	  
	  if test -n "$vers" && test "$vers" -ge $minvers; then
	  
          result="yes (version $crypto_version)"
	  
	  else
	  
	  result="no"
	  
          fi
	  
	  AC_MSG_RESULT($result)
          AC_SUBST(crypto_prefix)
else
	AC_MSG_CHECKING([whether to use embedded Crypto])
	AC_MSG_RESULT(yes)
	CRYPTO_PP_STYLE="embedded"
fi

AC_SUBST(CRYPTO_PP_STYLE)
])

AC_DEFUN([AM_OPTIONS_CRYPTO], [
	AC_ARG_WITH(
		[crypto-prefix],
		[AS_HELP_STRING(
			[--with-crypto-prefix=PREFIX],
			[prefix where crypto++ is installed])],
		[crypto_prefix="$withval"],
		[crypto_prefix=""])
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

AC_MSG_CHECKING([for bfd headers])
AC_RUN_IFELSE([
		AC_LANG_PROGRAM([[
			#include <ansidecl.h>
			#include <bfd.h>
			#include <stdio.h>
		]], [[
			FILE *f=fopen("conftestval", "w");
			if (!f) return 1;
			fprintf(f, "%s", "yes");
			fclose(f);
		]])
	], [
		if test -f conftestval; then
	        	result=`cat conftestval`
		else
			result=no
		fi
		AC_MSG_RESULT($result)
	], [
		result=no
		AC_MSG_RESULT($result)
	], [
		AC_MSG_RESULT([cross-compilation detected, checking only the header])
		AC_CHECK_HEADER(bfd.h, [result=yes], [result=no])
	])
if test x$result = xyes; then
	BFD_FLAGS="-DHAVE_BFD"
	BFD_LIB="-lbfd -liberty"
else
	AC_MSG_NOTICE([WARNING: bfd.h not found, please install binutils development package if you are a developer or want to help testing aMule])
fi

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
