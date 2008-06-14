dnl ----------------------------------------------------
dnl CHECK_WX_BUILT_WITH_GTK2
dnl check gtk version wx windows was compiled
dnl ----------------------------------------------------

AC_DEFUN(CHECK_WX_BUILT_WITH_GTK2,
[
  AC_MSG_CHECKING(if wxWindows was linked with GTK2)
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
dnl check wx windows 2.x version
dnl ----------------------------------------------------

AC_DEFUN(CHECK_WX_PARTIAL_VERSION,
[
  AC_MSG_CHECKING(if wxWindows version >=2.5.0 )
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
dnl get wx windows
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
  GTK2_VERSION=`$PKG_CONFIG --modversion gtk+-2.0`
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
AC_ARG_WITH(zlib,[  --with-zlib=DIR       use zlib in DIR],[
	if [ test "$withval" = "no" ]; then
		AC_MSG_ERROR([zlib is required by amule])
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
dnl Macros for wxWindows base detection. Typically used in configure.in as:
dnl
dnl 	AC_ARG_ENABLE(...)
dnl 	AC_ARG_WITH(...)
dnl	...
dnl	AM_OPTIONS_WXBASECONFIG
dnl	...
dnl	...
dnl	AM_PATH_WXBASECONFIG(2.3.4, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl     	   wxWindows must be installed on your system
dnl     	   but wx-config script couldn't be found.
dnl     
dnl     	   Please check that wx-config is in path, the directory
dnl     	   where wxWindows libraries are installed (returned by
dnl     	   'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl     	   equivalent variable and wxWindows version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WXBASE_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WXBASE_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WXBASE_CFLAGS_ONLY"
dnl     
dnl     LIBS="$LIBS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_WXBASECONFIG
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix and --wx-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN(AM_OPTIONS_WXBASECONFIG,
[
   AC_ARG_WITH(wxbase-prefix, [  --with-wxbase-prefix=PREFIX   Prefix where wxWindows base is installed (optional)],
               wxbase_config_prefix="$withval", wx_config_prefix="")
   AC_ARG_WITH(wxbase-exec-prefix,[  --with-wxbase-exec-prefix=PREFIX Exec prefix where wxWindowsbase  is installed (optional)],
               wxbase_config_exec_prefix="$withval", wxbase_config_exec_prefix="")
   AC_ARG_WITH(wxbase-config,[  --with-wxbase-config=CONFIG   wxbase-config script to use (optional)],
               wxbase_config_name="$withval", wxbase_config_name="")
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_WXBASECONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWindows, and define WX_C*FLAGS, WX_LIBS and WX_LIBS_STATIC
dnl (the latter is for static linking against wxWindows). Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wxbase-config script
dnl
AC_DEFUN(AM_PATH_WXBASECONFIG,
[
  dnl do we have wxbase-config name: it can be wxbase-config or wxd-config or ...
  if test x${WXBASE_CONFIG_NAME+set} != xset ; then
     WXBASE_CONFIG_NAME=wxbase-2.4-config
  fi
  if test "x$wxbase_config_name" != x ; then
     WXBASE_CONFIG_NAME="$wxbase_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wxbase_config_exec_prefix != x ; then
     wxbase_config_args="$wxbase_config_args --exec-prefix=$wxbase_config_exec_prefix"
     WXBASE_LOOKUP_PATH="$wxbase_config_exec_prefix/bin"
  fi
  if test x$wxbase_config_prefix != x ; then
     wxbase_config_args="$wxbase_config_args --prefix=$wxbase_config_prefix"
     WXBASE_LOOKUP_PATH="$WXBASE_LOOKUP_PATH:$wxbase_config_prefix/bin"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wxbase-config)
     WXBASE_CONFIG_PATH="$WXBASE_CONFIG_NAME"
     AC_MSG_RESULT($WXBASE_CONFIG_PATH)
  else
     AC_PATH_PROG(WXBASE_CONFIG_PATH, $WXBASE_CONFIG_NAME, no, "$WXBASE_LOOKUP_PATH:$PATH")
  fi

  if test "$WXBASE_CONFIG_PATH" != "no" ; then
    WXBASE_VERSION=""
    no_wxbase=""

    min_wxbase_version=ifelse([$1], ,2.2.1,$1)
    AC_MSG_CHECKING(for wxWindows base version >= $min_wxbase_version)

    WXBASE_CONFIG_WITH_ARGS="$WXBASE_CONFIG_PATH $wxbase_config_args"

    WXBASE_VERSION=`$WXBASE_CONFIG_WITH_ARGS --version`
    wxbase_config_major_version=`echo $WXBASE_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wxbase_config_minor_version=`echo $WXBASE_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wxbase_config_micro_version=`echo $WXBASE_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wxbase_requested_major_version=`echo $min_wxbase_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wxbase_requested_minor_version=`echo $min_wxbase_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wxbase_requested_micro_version=`echo $min_wxbase_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wxbase_ver_ok=""
    if test $wxbase_config_major_version -gt $wxbase_requested_major_version; then
      wxbase_ver_ok=yes
    else
      if test $wxbase_config_major_version -eq $wxbase_requested_major_version; then
         if test $wxbase_config_minor_version -gt $wxbase_requested_minor_version; then
            wxbase_ver_ok=yes
         else
            if test $wxbase_config_minor_version -eq $wxbase_requested_minor_version; then
               if test $wxbase_config_micro_version -ge $wxbase_requested_micro_version; then
                  wxbase_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test "x$wxbase_ver_ok" = x ; then
      no_wxbase=yes
    else
      WXBASE_LIBS=`$WXBASE_CONFIG_WITH_ARGS --libs`
      WXBASE_LIBS_STATIC=`$WXBASE_CONFIG_WITH_ARGS --static --libs`

      dnl starting with version 2.2.6 wxbase-config has --cppflags argument
      wxbase_has_cppflags=""
      if test $wxbase_config_major_version -gt 2; then
        wxbase_has_cppflags=yes
      else
        if test $wxbase_config_major_version -eq 2; then
           if test $wxbase_config_minor_version -gt 2; then
              wxbase_has_cppflags=yes
           else
              if test $wxbase_config_minor_version -eq 2; then
                 if test $wxbase_config_micro_version -ge 6; then
                    wxbase_has_cppflags=yes
                 fi
              fi
           fi
        fi
      fi

      if test "x$wxbase_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         WXBASE_CFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cflags`
         WXBASE_CPPFLAGS=$WXBASE_CFLAGS
         WXBASE_CXXFLAGS=$WXBASE_CFLAGS

         WXBASE_CFLAGS_ONLY=$WXBASE_CFLAGS
         WXBASE_CXXFLAGS_ONLY=$WXBASE_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
         WXBASE_CPPFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cppflags`
         WXBASE_CXXFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cxxflags`
         WXBASE_CFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cflags`

         WXBASE_CFLAGS_ONLY=`echo $WXBASE_CFLAGS | sed "s@^$WXBASE_CPPFLAGS *@@"`
         WXBASE_CXXFLAGS_ONLY=`echo $WXBASE_CXXFLAGS | sed "s@^$WXBASE_CFLAGS *@@"`
      fi
    fi

    if test "x$no_wxbase" = x ; then
       AC_MSG_RESULT(yes (version $WXBASE_VERSION))
       ifelse([$2], , :, [$2])
    else
       if test "x$WXBASE_VERSION" = x; then
	  dnl no wxbase-config at all
	  AC_MSG_RESULT(no)
       else
	  AC_MSG_RESULT(no (version $WXBASE_VERSION is not new enough))
       fi

       WXBASE_CFLAGS=""
       WXBASE_CPPFLAGS=""
       WXBASE_CXXFLAGS=""
       WXBASE_LIBS=""
       WXBASE_LIBS_STATIC=""
       ifelse([$3], , :, [$3])
    fi
  fi

  AC_SUBST(WXBASE_CPPFLAGS)
  AC_SUBST(WXBASE_CFLAGS)
  AC_SUBST(WXBASE_CXXFLAGS)
  AC_SUBST(WXBASE_CFLAGS_ONLY)
  AC_SUBST(WXBASE_CXXFLAGS_ONLY)
  AC_SUBST(WXBASE_LIBS)
  AC_SUBST(WXBASE_LIBS_STATIC)
  AC_SUBST(WXBASE_VERSION)
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
 check="7.9.7"
 dnl check_hex must be UPPERCASE if any hex letters are present
 check_hex="070907"


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
	OWN_CRYPTO="yes"
	if test x$crypto_prefix == x ; then
	crypto_prefix="/usr/include/"
	OWN_CRYPTO="no"
	fi
	grep "5.1" $crypto_prefix/crypto++/cryptlib.h > /dev/null 2>&1
	CRYPTO=$?
	if test "$CRYPTO" != 0; then
	grep "5.1" $crypto_prefix/cryptopp/cryptlib.h > /dev/null 2>&1
	CRYPTO=$?
	fi
	if test "$CRYPTO" != 0; then
		result="no"
	else
		result="yes"
	fi
	AC_MSG_RESULT($result)
	AC_SUBST(CRYPTO)
	AC_SUBST(OWN_CRYPTO)
	AC_SUBST(crypto_prefix)
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