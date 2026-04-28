if (NOT YYENABLE_NLS)
	include (CheckIncludeFile)

	# These header probes get recorded into config.h.cm as HAVE_* macros for
	# completeness (autotools historically did the same), but aMule does not
	# itself include any of them — translation is delegated to wxLocale,
	# which links libintl via wxWidgets.  argz.h and nl_types.h in particular
	# are glibc-only, so gating ENABLE_NLS on them silently disabled
	# localization on every non-glibc platform (macOS, MinGW/MSYS2, *BSD)
	# even when a working gettext was installed.  Probe but do not gate.
	check_include_file (argz.h HAVE_ARGZ_H)
	check_include_file (limits.h HAVE_LIMITS_H)
	check_include_file (locale.h HAVE_LOCALE_H)
	check_include_file (nl_types.h HAVE_NL_TYPES_H)
	check_include_file (stdlib.h HAVE_STDLIB_H)
	check_include_file (string.h HAVE_STRING_H)
	check_include_file (sys/param.h HAVE_SYS_PARAM_H)
	if (NOT HAVE_STDDEF_H)
		check_include_file (stddef.h HAVE_STDDEF_H)
	endif()

	# Homebrew's gettext is keg-only (not symlinked into /opt/homebrew/bin)
	# because it would shadow macOS's BSD gettext-runtime in libc.  Pre-find
	# msgfmt / msgmerge under the keg so users don't have to set PATH
	# manually before invoking cmake.  Skipped if the user already exported
	# PATH or pointed cmake at a different gettext.
	if (APPLE AND NOT GETTEXT_MSGFMT_EXECUTABLE)
		find_program (GETTEXT_MSGFMT_EXECUTABLE msgfmt
			HINTS /opt/homebrew/opt/gettext/bin /usr/local/opt/gettext/bin)
	endif()
	if (APPLE AND NOT GETTEXT_MSGMERGE_EXECUTABLE)
		find_program (GETTEXT_MSGMERGE_EXECUTABLE msgmerge
			HINTS /opt/homebrew/opt/gettext/bin /usr/local/opt/gettext/bin)
	endif()

	include (FindGettext)

	# msgfmt / msgmerge are required at build time for compiling the .po
	# catalogs into .mo files installed alongside the binaries.
	if (NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT GETTEXT_MSGMERGE_EXECUTABLE)
		set (ENABLE_NLS FALSE)
	endif()

	# Parser.cpp and the webserver call dgettext directly, so libintl needs
	# to be on the link line.  On glibc systems Intl_LIBRARIES is empty
	# (gettext lives inside libc); on macOS, MinGW/MSYS2 and *BSD it points
	# at the separate libintl.  Without this, ENABLE_NLS=ON used to link
	# fine on Linux and fail with `_libintl_dgettext` undefined elsewhere.
	if (ENABLE_NLS)
		find_package (Intl)
		if (NOT Intl_FOUND)
			message (STATUS "libintl headers/library not found — disabling NLS")
			set (ENABLE_NLS FALSE)
		endif()
	endif()

	if (ENABLE_NLS)
		message (STATUS "Everything is fine. aMule can be localized")
		set (YYENABLE_NLS TRUE CACHE INTERNAL "For parser, php-parser and to not recheck for nls-support" FORCE)
	else()
		message (STATUS "You need to install GNU gettext/gettext-tools to compile aMule with i18n support.")
	endif()
endif()
