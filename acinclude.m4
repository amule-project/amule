dnl ----------------------------------------------------
dnl CHECK_WX_BUILT_WITH_GTK2
dnl check gtk version wx widgets was compiled
dnl ----------------------------------------------------

AC_DEFUN(CHECK_WX_BUILT_WITH_GTK2,
[
  AC_MSG_CHECKING(if wxWidgets was linked with GTK2)
  if $WX_CONFIG_NAME --cppflags | grep -q 'gtk2' ; then
     GTK_USEDVERSION=2
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi

  AC_SUBST(GTK_USEDVERSION)
])

dnl ----------------------------------------------------
dnl CHECK_WX_PARTIAL_VERSION
dnl check wx widgets 2.x version
dnl ----------------------------------------------------

AC_DEFUN(CHECK_WX_PARTIAL_VERSION,
[
  AC_MSG_CHECKING(if wxWidgets version >=2.5.0 )
  if $WX_CONFIG_NAME --version | grep -q '2.5' ; then
     WX_PARTIAL_VERSION=5
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi
  AC_SUBST(WX_PARTIAL_VERSION)
])

dnl ----------------------------------------------------
dnl GET_WXGTK_VERSION
dnl get wx widgets
dnl ----------------------------------------------------

AC_DEFUN(GET_WXGTK_VERSION,
[
  WXGTK_VERSION=`$WX_CONFIG_NAME --version`
  AC_SUBST(WXGTK_VERSION)
])


dnl ----------------------------------------------------
dnl GET_GTK_VERSION
dnl get gtk 1.x version
dnl ----------------------------------------------------

AC_DEFUN(GET_GTK_VERSION,
[
  GTK_VERSION=`$GTK_CONFIG --version`
  AC_SUBST(GTK_VERSION)
])

dnl ----------------------------------------------------
dnl GET_GTK2_VERSION
dnl get gtk 2.x version
dnl ----------------------------------------------------

AC_DEFUN(GET_GTK2_VERSION,
[
  GTK_VERSION=`$PKG_CONFIG --modversion gtk+-2.0`
  AC_SUBST(GTK_VERSION)
])

dnl ----------------------------------------------------
dnl CHECK_ZLIB
dnl check if zlib is on the system
dnl ----------------------------------------------------
AC_DEFUN(CHECK_ZLIB,
[
wv_zlib=""
found_zlib="no"

ZLIB_DIR=""
ZVERMAX="1"
ZVERMED="1"
ZVERMIN="4"
AC_ARG_WITH(zlib,[  --with-zlib=DIR       use zlib in DIR],[
	if [ test "$withval" = "no" ]; then
		AC_MSG_ERROR([zlib is required by aMule])
        elif [ test "$withval" = "yes" ]; then
		zlib=check
        elif [ test "$withval" = "peer" ]; then
		zlib=peer
	else
		zlib=sys
		ZLIB_DIR="$withval"
		wv_zlib="--with-zlib=$withval"
        fi
],[	zlib=check
])

if test $zlib = peer; then
	z=peer
else
	if test $zlib = sys; then
		_cppflags="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -I$ZLIB_DIR/include"
	fi
	AC_CHECK_HEADER(zlib.h,[
		z=sys
        	CZVER=`grep "define ZLIB_VERSION" /usr/include/zlib.h|sed 's/#define ZLIB_VERSION "//' |sed 's/"//'`
		CZMIN=`echo $CZVER |sed 's/....//'`
		CZMED=`echo $CZVER |sed s/.$CZMIN//| sed 's/^..//'`
		CZMAX=`echo $CZVER |sed s/...$CZMIN//`
		if test ["$CZMAX" -lt "$ZVERMAX"]; then
		        result="no"
                	AC_MSG_ERROR([ zlib >=1.1.4 is required by aMule])
		fi
		if test [ "$CZMED" -lt "$ZVERMED"]; then
				result="no"
                		AC_MSG_ERROR([ zlib >=1.1.4 is required by aMule])
		fi;
		
	],[	if test $zlib = sys; then
			AC_MSG_ERROR([zlib not found in system location])
		fi
		z=peer
	])
	if test $zlib = sys; then
		CPPFLAGS="$_cppflags"
	fi
fi

if test $z = peer; then
	AC_MSG_CHECKING(for zlib in peer directory)
	if test -d ../zlib; then
		if test -r ../zlib/libz.a; then
			AC_MSG_RESULT(yes)
			CZVER=`grep "define ZLIB_VERSION" ../include/zlib.h|sed 's/#define ZLIB_VERSION "//' |sed 's/"//'`
		CZMIN=`echo $CZVER |sed 's/....//'`
		CZMED=`echo $CZVER |sed s/.$CZMIN//| sed 's/^..//'`
		CZMAX=`echo $CZVER |sed s/...$CZMIN//`
		if test ["$CZMAX" -lt "$ZVERMAX"]; then
		        result="no"
                	AC_MSG_ERROR([ zlib >=1.1.4 is required by amule])
		fi
		if test [ "$CZMED" -lt "$ZVERMED"]; then
				result="no"
                		AC_MSG_ERROR([ zlib >=1.1.4 is required by aMule])
		fi;
        	
		else
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([unable to use peer zlib - zlib/libz.a not found])
		fi
	else
		AC_MSG_RESULT(no)
		AC_MSG_ERROR([unable to use zlib - no peer found])
	fi

	zlib_message="peer zlib"
	ZLIB_CFLAGS='-I$(top_srcdir)/../zlib'
	ZLIB_LIBS='$(top_srcdir)/../zlib/libz.a'

	wv_cppflags="$wv_cppflags -I$ZLIB_PEERDIR"
else
	if test $zlib = sys; then
		zlib_message="zlib in -L$ZLIB_DIR/lib -lz"
		ZLIB_CFLAGS="-I$ZLIB_DIR/include"
		ZLIB_LIBS="-L$ZLIB_DIR/lib -lz"
	else
		zlib_message="zlib in -lz"
		ZLIB_CFLAGS=""
		ZLIB_LIBS="-lz"
	fi
fi

AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)

])

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_GDLIBCONFIG
dnl
dnl adds support for --gdlib-prefix and --gdlib-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN(AM_OPTIONS_GDLIBCONFIG,
[
   AC_ARG_WITH(gdlib-prefix, [  --with-gdlib-prefix=PREFIX   Prefix where gdlib is installed (optional)],
               gdlib_config_prefix="$withval", gdlib_config_prefix="")
   AC_ARG_WITH(gdlib-exec-prefix,[  --with-gdlib-exec-prefix=PREFIX Exec prefix where gdlib  is installed (optional)],
               gdlib_config_exec_prefix="$withval", gdlib_config_exec_prefix="")
   AC_ARG_WITH(gdlib-config,[  --with-gdlib-config=CONFIG   gdlib-config script to use (optional)],
               gdlib_config_name="$withval", gdlib_config_name="")
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
AC_DEFUN(AM_PATH_GDLIBCONFIG,
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
     gdlib_config_args="$gdlib_config_args --exec-prefix=$gdlib_config_exec_prefix"
     GDLIB_LOOKUP_PATH="$gdlib_config_exec_prefix/bin"
  fi
  if test x$gdlib_config_prefix != x ; then
     gdlib_config_args="$gdlib_config_args --prefix=$gdlib_config_prefix"
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
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gdlib_config_minor_version=`echo $GDLIB_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gdlib_config_micro_version=`echo $GDLIB_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    gdlib_requested_major_version=`echo $min_gdlib_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gdlib_requested_minor_version=`echo $min_gdlib_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gdlib_requested_micro_version=`echo $min_gdlib_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

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
       ifelse([$2], , :, [$2])
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

dnl **************************
dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_CURLCONFIG
dnl
dnl adds support for curl-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN(AM_OPTIONS_CURLCONFIG,
[
   AC_ARG_WITH(curl-config,[  --with-curl-config=CONFIG   curl-config script to use (optional)],
               curl_config_name="$withval", curl_config_name="")

   AC_ARG_WITH(curl-prefix,[  --with-curl-prefix=PFX   Prefix where curl is installed (optional) (unused)],
            curl_config_prefix="$withval", curl_config_prefix="")
])


dnl ---------------------------------------------------------------------------
dnl AM_PATH_CURLCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Check curl-config
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the curl-config script
dnl

AC_DEFUN(AM_PATH_CURLCONFIG,[
 AC_CACHE_VAL(my_cv_curl_vers,[
 my_cv_curl_vers=NONE
 dnl check is the plain-text version of the required version
 check="7.9.5"
 dnl check_hex must be UPPERCASE if any hex letters are present
 check_hex="070905"


 AC_MSG_CHECKING([for curl >= $check])

  if test x${CURL_CONFIG_NAME+set} != xset ; then
     CURL_CONFIG_NAME=curl-config
  fi
  if test "x$curl_config_name" != x ; then
     CURL_CONFIG_NAME="$curl_config_name"
  fi

  if test x$curl_config_prefix != x ; then
     curl_config_args="$curl_config_args --prefix=$curl_config_prefix"
     if test x${CURL_CONFIG_NAME+set} != xset ; then
        CURL_CONFIG_NAME="$curl_config_prefix/bin/curl-config"
     fi
  fi

## We cannot do that until curl-config allow us to use --prefix=PFX
##  CURL_LIBS="`$CURL_CONFIG_NAME $curl_config_args --libs`"
##  CURL_FLAGS="`$CURL_CONFIG_NAME $curl_config_args --cflags`"

# Fix for buggy curl config on non-i386
  CURL_LIBS="`$CURL_CONFIG_NAME --libs | sed "s/-arch i386//g"`"
  CURL_FLAGS="`$CURL_CONFIG_NAME --cflags`"

 if eval $CURL_CONFIG_NAME --version 2>/dev/null >/dev/null; then
   ver=`$CURL_CONFIG_NAME --version | sed -e "s/libcurl //g"`
   hex_ver=`$CURL_CONFIG_NAME --vernum | tr 'a-f' 'A-F'`
   ok=`echo "ibase=16; if($hex_ver>=$check_hex) $hex_ver else 0" | bc`

   if test x$ok != x0; then
     my_cv_curl_vers="$ver"
     AC_MSG_RESULT(yes (version $my_cv_curl_vers))
     CURLFOUND=1
     AC_SUBST(CURL_LIBS)
     AC_SUBST(CURL_FLAGS)
   else
     AC_MSG_RESULT(no)
     CURLFOUND=0
     AC_MSG_WARN([Curl version $ver is too old. Need version $check or higher.])
   fi
 else
   AC_MSG_RESULT(no)
   CURLFOUND=0
   AC_MSG_WARN([curl-config was not found])
 fi
 AC_SUBST(CURLFOUND)
 ])
])


dnl --------------------------------------------------------------------------
dnl Check for crypto++ library
dnl --------------------------------------------------------------------------

AC_DEFUN(CHECK_CRYPTO,
	[
     AC_MSG_CHECKING([for crypto++ version >= 5.1])
	if test x$crypto_prefix == x ; then
	crypto_prefix="/usr/include/"
	fi
	CRYPTO_PP_STYLE="gentoo_debian"
	grep "5.1" $crypto_prefix/crypto++/cryptlib.h > /dev/null 2>&1
	CRYPTO=$?
	if test "$CRYPTO" != 0; then
	grep "5.1" $crypto_prefix/cryptopp/cryptlib.h > /dev/null 2>&1
	CRYPTO_PP_STYLE="mdk_suse_fc"
	CRYPTO=$?
	fi
	if test "$CRYPTO" != 0; then
	grep "5.1" $crypto_prefix/crypto-5.1/cryptlib.h > /dev/null 2>&1
	CRYPTO_PP_STYLE="sources"
	CRYPTO=$?
	fi
	if test "$CRYPTO" != 0; then
		result="no"
	else
		result="yes"
	fi
	AC_MSG_RESULT($result)
	AC_SUBST(CRYPTO)
	AC_SUBST(crypto_prefix)
	AC_SUBST(CRYPTO_PP_STYLE)
	])

AC_DEFUN(AM_OPTIONS_CRYPTO,
[
   AC_ARG_WITH( crypto-prefix,[  --with-crypto-prefix=PFX   Prefix where crypto++ is installed (optional)],
            crypto_prefix="$withval", crypto_prefix="")
])

dnl --------------------------------------------------------------------------
dnl CCache support
dnl --------------------------------------------------------------------------

AC_DEFUN(CHECK_CCACHE,
	[
	if test x$ccache_prefix == x ; then
	ccache_prefix="/usr/bin"
	fi
	$ccache_prefix/ccache-config --version > /dev/null 2>&1
	CCACHE=$?
	if test "$CCACHE" != 0; then
        ccache_prefix="/usr/local/bin"
	fi
	$ccache_prefix/ccache-config --version > /dev/null 2>&1
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


AC_DEFUN(AM_OPTIONS_CCACHE_PFX,
[
   AC_ARG_WITH( ccache-prefix,[  --with-ccache-prefix=PFX   Prefix where ccache is installed (optional)],
            ccache_prefix="$withval", ccache_prefix="")
])
