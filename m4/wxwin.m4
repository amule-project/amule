dnl ---------------------------------------------------------------------------
dnl Macros for wxWidgets detection. Typically used in configure.in as:
dnl
dnl 	AC_ARG_ENABLE(...)
dnl 	AC_ARG_WITH(...)
dnl	...
dnl	AM_OPTIONS_WXCONFIG
dnl	...
dnl	...
dnl	AM_PATH_WXCONFIG(2.3.4, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl     	   wxWidgets must be installed on your system
dnl     	   but wx-config script couldn't be found.
dnl     
dnl     	   Please check that wx-config is in path, the directory
dnl     	   where wxWidgets libraries are installed (returned by
dnl     	   'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl     	   equivalent variable and wxWidgets version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl     
dnl     LDFLAGS="$LDFLAGS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_WXCONFIG
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix and --wx-config 
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXCONFIG],
[
   AC_ARG_WITH(wx-prefix, [  --with-wx-prefix=PREFIX          prefix where wxWidgets is installed],
               wx_config_prefix="$withval", wx_config_prefix="")
   AC_ARG_WITH(wx-exec-prefix,[  --with-wx-exec-prefix=PREFIX     exec prefix where wxWidgets is installed],
               wx_config_exec_prefix="$withval", wx_config_exec_prefix="")
   AC_ARG_WITH(wx-config,[  --with-wx-config=CONFIG          wx-config script to use],
               wx_config_name="$withval", wx_config_name="")
])

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_WXBASECONFIG
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix and --wx-config
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXBASECONFIG],
[
   AC_ARG_WITH(wxbase-prefix, [  --with-wxbase-prefix=PREFIX      prefix where wxWidgets base is installed],
               wxbase_config_prefix="$withval", wxbase_config_prefix="")
   AC_ARG_WITH(wxbase-exec-prefix,[  --with-wxbase-exec-prefix=PREFIX exec prefix where wxWidgetsbase  is installed],
               wxbase_config_exec_prefix="$withval", wxbase_config_exec_prefix="")
   AC_ARG_WITH(wxbase-config,[  --with-wxbase-config=CONFIG      wxbase-config script to use],
               wxbase_config_name="$withval", wxbase_config_name="")
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_WXCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWidgets, and define WX_C*FLAGS and WX_LIBS (this will also be
dnl used for static linking against wxWidgets). Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wx-config script
dnl
AC_DEFUN([AM_PATH_WXCONFIG],
[
  AC_REQUIRE([AC_PROG_AWK])

  dnl do we have wx-config name: it can be wx-config or wxd-config or ...
  if test x${WX_CONFIG_NAME+set} != xset ; then
     WX_CONFIG_NAME=wx-config
  fi
  if test "x$wx_config_name" != x ; then
     WX_CONFIG_NAME="$wx_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     WX_LOOKUP_PATH="$wx_config_exec_prefix/bin"
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     WX_LOOKUP_PATH="$WX_LOOKUP_PATH:$wx_config_prefix/bin"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wx-config)
     WX_CONFIG_PATH="$WX_CONFIG_NAME"
     AC_MSG_RESULT($WX_CONFIG_PATH)
  else
     AC_PATH_PROG(WX_CONFIG_PATH, $WX_CONFIG_NAME, no, "$WX_LOOKUP_PATH:$PATH")
  fi

  if test "$WX_CONFIG_PATH" != "no" ; then
    WX_VERSION=""
    no_wx=""

    min_wx_version=ifelse([$1], ,2.4.2,$1)

    WX_CONFIG_WITH_ARGS="$WX_CONFIG_PATH $wx_config_args"

    WX_VERSION=`$WX_CONFIG_WITH_ARGS --version`
    
    vers=`echo $WX_VERSION | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
    minvers=`echo $min_wx_version | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`

    if test "x${need_gui:-yes}" = "xyes" ; then
    
      AC_MSG_CHECKING(for wxWidgets version >= $min_wx_version)
      if test -n "$vers" && test "$vers" -ge $minvers; then

        case "$USE_DEBUG_STATIC" in
          yes)
            WX_LIBS=`$WX_CONFIG_WITH_ARGS --static --libs`
            ;;
          *)
            WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs`
            ;;
        esac

        dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
        WX_CPPFLAGS=`$WX_CONFIG_WITH_ARGS --cppflags`
        WX_CXXFLAGS=`$WX_CONFIG_WITH_ARGS --cxxflags`
        WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags`

        WX_CFLAGS_ONLY=`echo $WX_CFLAGS | sed "s@^$WX_CPPFLAGS *@@"`
        WX_CXXFLAGS_ONLY=`echo $WX_CXXFLAGS | sed "s@^$WX_CFLAGS *@@"`
      else
        no_wx=yes
      fi

      if test "x$no_wx" = x ; then
        AC_MSG_RESULT(yes (version $WX_VERSION))
        ifelse([$2], , :, [$2])
      else
        if test "x$WX_VERSION" = x; then
          dnl no wx-config at all
          AC_MSG_RESULT(no)
        else
          AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
        fi

        WX_CFLAGS=""
        WX_CPPFLAGS=""
        WX_CXXFLAGS=""
        WX_LIBS=""
        ifelse([$3], , :, [$3])

        AC_MSG_ERROR([
         Please check that wx-config is in path, the directory
         where wxWidgets libraries are installed (returned by
         'wx-config --libs' command) is in LD_LIBRARY_PATH or
         equivalent variable and wxWidgets version is new enough.
         Or this might also be a bug in our configure. Please try again
         with --with-wx-config=/usr/bin/wx-config
         (replace /usr/bin/wx-config with a valid path to your wx-config)
         * Note:
         Most probably, either one of the above aren't correct, you don't
         have wxGTK installed, or are missing wxGTK-devel (or equivalent) package.
         ])
      fi

    else

      if test "$vers" -ge 2005000; then
        AC_MSG_CHECKING(for wxWidgets version >= $min_wx_version)
        if test "x$no_wx" = x ; then
          AC_MSG_RESULT(yes (version $WX_VERSION))
        else
          if test "x$WX_VERSION" = x; then
            dnl no wx-config at all
            AC_MSG_RESULT(no)
          else
            AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
          fi
        fi
      fi
      WX_CFLAGS=""
      WX_CPPFLAGS=""
      WX_CXXFLAGS=""
      WX_LIBS=""

    fi
  else
    AC_MSG_ERROR([
	You need wxWidgets (http://www.wxwidgets.org/) to compile aMule. If you
	have wxWidgets installed, please check that wx-config is in path, and
	the directory where wxWidgets libraries are installed is in
	LD_LIBRARY_PATH or equivalent variable. If you have wx-config in a
	non-standard location please use the --with-wx-config=/path/to/wx-config
	configure option.
	])
  fi
  
  AC_SUBST(WX_CPPFLAGS)
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_CXXFLAGS)
  AC_SUBST(WX_CFLAGS_ONLY)
  AC_SUBST(WX_CXXFLAGS_ONLY)
  AC_SUBST(WX_LIBS)
  AC_SUBST(WX_VERSION)
  
  dnl Now checking if it is a 2.5 or more version
  dnl Then take wx-config itself
  
  if test "$vers" -ge 2005000; then
    if test "$vers" -ge 2005003; then
      wx_conig_base_libs="--libs base,net";
    else
      wx_conig_base_libs="--libs=base,net";
    fi
    AC_MSG_WARN(wxWidgets >=2.5.0: Using wx-config ${wx_conig_base_libs})
  
    WXBASE_CONFIG_NAME=$WX_CONFIG_NAME
    WXBASE_CONFIG_WITH_ARGS=$WX_CONFIG_WITH_ARGS

    case "$USE_DEBUG_STATIC" in
	  yes)
        WXBASE_LIBS=`$WXBASE_CONFIG_WITH_ARGS --static ${wx_conig_base_libs}`
		;;
	 *)
        WXBASE_LIBS=`$WXBASE_CONFIG_WITH_ARGS ${wx_conig_base_libs}`
		;;
	esac
      
    dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
    WXBASE_CPPFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cppflags`
    WXBASE_CXXFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cxxflags`
    WXBASE_CFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cflags`

    WXBASE_CFLAGS_ONLY=`echo $WXBASE_CFLAGS | sed "s@^$WXBASE_CPPFLAGS *@@"`
    WXBASE_CXXFLAGS_ONLY=`echo $WXBASE_CXXFLAGS | sed "s@^$WXBASE_CFLAGS *@@"`
  
    WXBASE24FOUND=0
    WXBASE25FOUND=1
  
    AC_SUBST(WXBASE_CPPFLAGS)
    AC_SUBST(WXBASE_CFLAGS)
    AC_SUBST(WXBASE_CXXFLAGS)
    AC_SUBST(WXBASE_CFLAGS_ONLY)
    AC_SUBST(WXBASE_CXXFLAGS_ONLY)
    AC_SUBST(WXBASE_LIBS)
    AC_SUBST(WXBASE_VERSION)
  else
  
    dnl For wx  < 2.5.0, looking for wxbase-config
  
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

    dnl don't search the PATH if WXBASE_CONFIG_NAME is absolute filename
    if test -x "$WXBASE_CONFIG_NAME" ; then
       AC_MSG_CHECKING(for wxbase-config)
       WXBASE_CONFIG_PATH="$WXBASE_CONFIG_NAME"
       AC_MSG_RESULT($WXBASE_CONFIG_PATH)
    else
       AC_PATH_PROG(WXBASE_CONFIG_PATH, $WXBASE_CONFIG_NAME, no, "$WXBASE_LOOKUP_PATH:$PATH")
    fi

    if test "$WXBASE_CONFIG_PATH" != "no" ; then
      WXBASE_VERSION=""
      no_wxbase=""

      min_wxbase_version=ifelse([$1], ,2.4.2,$1)
      AC_MSG_CHECKING(for wxWidgets base version >= $min_wxbase_version)

      WXBASE_CONFIG_WITH_ARGS="$WXBASE_CONFIG_PATH $wxbase_config_args"

      WXBASE_VERSION=`$WXBASE_CONFIG_WITH_ARGS --version`
      vers=`echo $WXBASE_VERSION | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
      minvers=`echo $min_wxbase_version | $AWK 'BEGIN { FS = "."; } { printf "% d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
    
      if test -n "$vers" && test "$vers" -ge $minvers; then
	WXBASE24FOUND=1
        WXBASE25FOUND=0

        case "$USE_DEBUG_STATIC" in
		  yes)
            WXBASE_LIBS=`$WXBASE_CONFIG_WITH_ARGS --static --libs`
			;;
		  *)
            WXBASE_LIBS=`$WXBASE_CONFIG_WITH_ARGS --libs`
			;;
		esac

        dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
        WXBASE_CPPFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cppflags`
        WXBASE_CXXFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cxxflags`
        WXBASE_CFLAGS=`$WXBASE_CONFIG_WITH_ARGS --cflags`

        WXBASE_CFLAGS_ONLY=`echo $WXBASE_CFLAGS | sed "s@^$WXBASE_CPPFLAGS *@@"`
        WXBASE_CXXFLAGS_ONLY=`echo $WXBASE_CXXFLAGS | sed "s@^$WXBASE_CFLAGS *@@"`
      else 
        no_wxbase=yes    
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

	 WXBASE24FOUND=0
         WXBASE25FOUND=0
  
         WXBASE_CFLAGS=""
         WXBASE_CPPFLAGS=""
         WXBASE_CXXFLAGS=""
         WXBASE_LIBS=""
         ifelse([$3], , :, [$3])
	 
         AC_MSG_NOTICE([
	  WARNING: A usable libwx_base was not found.
	  This is not needed for compiling aMule, but it is for compiling amulecmd.
	  If you have no wxbase, amulecmd will be linked against wxgtk, thus removing
	  all the non-graphical client meaning at all.
          Please check that wxbase-2.X-config is in path, the directory
          where wxWidgets base libraries are installed (returned by
          'wxbase-2.X-config --libs' command) is in LD_LIBRARY_PATH or
          equivalent variable and wxWidgets base version is new enough.
	  Or this might also be that your wxbase-config has other name.
	  Please try again with --with-wxbase-config=/usr/bin/wxbase-config
	  (replace /usr/bin/wxbase-config with a valid path to your wxbase-config)
	  * Note:
	  Most probably, either one of the above aren't correct, you don't
	  have wxbase installed, or are missing wxbase-devel (or equivalent) package.
	 ])
      fi
    fi
    
    AC_SUBST(WXBASE_CPPFLAGS)
    AC_SUBST(WXBASE_CFLAGS)
    AC_SUBST(WXBASE_CXXFLAGS)
    AC_SUBST(WXBASE_CFLAGS_ONLY)
    AC_SUBST(WXBASE_CXXFLAGS_ONLY)
    AC_SUBST(WXBASE_LIBS)
    AC_SUBST(WXBASE_VERSION)
  fi
])


dnl ---------------------------------------------------------------------------
dnl AM_WXCONFIG_LARGEFILE()
dnl
dnl Test that wxWidgets is built with support for large-files. If not
dnl configure is terminated. The WX_CFLAGS variables
dnl ---------------------------------------------------------------------------
AC_DEFUN([AM_WXCONFIG_LARGEFILE],
[
	AC_LANG_PUSH(C++)
	
	if test "x${no_wx}" == "x";
	then
		dnl Backup current flags
		__CFLAGS=${CFLAGS}
		__CXXFLAGS=${CXXFLAGS}
		__LIBS=${LIBS}

		dnl Use wx-flags for testing
		CFLAGS=${WX_CFLAGS}
		CXXFLAGS=${WX_CXXFLAGS}
		LIBS=${WX_LIBS}
		
		AC_MSG_CHECKING(that wxWidgets has support for large files)
		AC_TRY_RUN([
			#include <wx/wx.h>

			int main() {
			#if HAVE_LARGEFILE_SUPPORT
				exit(0);
			#else
				exit(1);
			#endif
			}
		], , NO_LF="true", )

    	if test "x${NO_LF}" != "x";
		then
			AC_MSG_RESULT(no)
    		AC_MSG_ERROR([
			Support for large files in wxWidgets is required by aMule.
			To continue you must recompile wxWidgets with support for 
			large files enabled.
			])
		else
			AC_MSG_RESULT(yes)
		fi

		dnl Restore backup'd flags
		CFLAGS=${__CFLAGS}
		CXXFLAGS=${__CXXFLAGS}
		LIBS=${__LIBS}
	fi
	
	dnl Test wxBase
	if test "x${no_wxbase}" == "x";
	then
		dnl Backup current flags
		__CFLAGS=${CFLAGS}
		__CXXFLAGS=${CXXFLAGS}
		__LIBS=${LIBS}

		dnl Use wx-flags for testing
		CFLAGS=${WXBASE_CFLAGS}
		CXXFLAGS=${WXBASE_CXXFLAGS}
		LIBS=${WXBASE_LIBS}	
		
		AC_MSG_CHECKING(that wxBase has support for large files)
		AC_TRY_RUN([
			#include <wx/wx.h>

			int main() {
			#if HAVE_LARGEFILE_SUPPORT
				exit(0);
			#else
				exit(1);
			#endif
			}
		], , NO_LF="true", )

    	if test "x${NO_LF}" != "x";
		then
			AC_MSG_RESULT(no)
    		AC_MSG_ERROR([
			Support for large files in wxBase is required by aMule.
			To continue you must recompile wxWidgets with support for 
			large files enabled.
			])
		else
			AC_MSG_RESULT(yes)
		fi

		dnl Restore backup'd flags
		CFLAGS=${__CFLAGS}
		CXXFLAGS=${__CXXFLAGS}
		LIBS=${__LIBS}
	fi

	AC_LANG_POP(C++)
])

