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
	# catalogs into .mo files installed alongside the binaries. This file
	# is only included from the top-level CMakeLists.txt when ENABLE_NLS
	# is true — i.e. the user explicitly asked for the feature. Honour
	# the user's intent: fail loudly instead of silently downgrading to
	# ENABLE_NLS=FALSE, which would mask the missing dep behind a green
	# build with no localization at runtime.
	if (NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT GETTEXT_MSGMERGE_EXECUTABLE)
		message (FATAL_ERROR "ENABLE_NLS=YES but msgfmt and/or msgmerge were "
			"not found. Install GNU gettext (Debian/Ubuntu: gettext, "
			"Fedora: gettext, macOS Homebrew: gettext, "
			"MSYS2: mingw-w64-x86_64-gettext), or pass -DENABLE_NLS=NO to "
			"build without translation support.")
	endif()

	message (STATUS "Everything is fine. aMule can be localized")
	set (YYENABLE_NLS TRUE CACHE INTERNAL "For parser, php-parser and to not recheck for nls-support" FORCE)
endif()

# Parser.cpp and the webserver call dgettext directly, so libintl needs
# to be on the link line.  On glibc systems Intl_LIBRARIES is empty
# (gettext lives inside libc); on macOS, MinGW/MSYS2 and *BSD it points
# at the separate libintl.  Without this, ENABLE_NLS=ON used to link
# fine on Linux and fail with `_libintl_dgettext` undefined elsewhere.
#
# This block lives *outside* the YYENABLE_NLS gate above intentionally.
# find_package(Intl) populates Intl_FOUND / Intl_LIBRARIES / Intl_INCLUDE_DIRS
# as ordinary (non-cache) variables, so on every fresh `cmake` configure
# pass they evaporate unless re-derived.  The cache-gated upper branch
# skips on subsequent configures (e.g. when a SVNDATE refresh triggers
# CMAKE_CONFIGURE_DEPENDS), which silently dropped the libintl link
# wiring in src/CMakeLists.txt's `if (ENABLE_NLS AND Intl_FOUND)` guard
# and produced `_libintl_dgettext` undefined-symbol failures on the
# next incremental build.  Calling find_package(Intl) every configure
# is cheap: its own internal cache (Intl_INCLUDE_DIR / Intl_LIBRARY)
# is preserved, so subsequent runs are effectively a cache lookup.
if (ENABLE_NLS)
	find_package (Intl)
	if (NOT Intl_FOUND)
		# Same reasoning as the msgfmt branch above: ENABLE_NLS=YES is
		# the user's explicit intent, so the missing libintl is a hard
		# error, not a silent fallback. On glibc systems Intl_LIBRARIES
		# is empty because gettext lives inside libc — find_package(Intl)
		# still reports Intl_FOUND=TRUE there. So this branch only fires
		# on platforms with separate libintl (macOS, MinGW/MSYS2, *BSD,
		# musl) where the runtime hasn't been installed.
		message (FATAL_ERROR "ENABLE_NLS=YES but the libintl headers/library "
			"were not found. Install GNU gettext's runtime "
			"(macOS Homebrew: gettext, MSYS2: mingw-w64-x86_64-gettext, "
			"Alpine/musl: gettext-dev + musl-libintl, *BSD: gettext-runtime), "
			"or pass -DENABLE_NLS=NO to build without translation support. "
			"Linux glibc systems should not hit this branch — gettext is "
			"part of libc and find_package(Intl) succeeds with empty "
			"Intl_LIBRARIES; if you do hit it on a glibc system, your "
			"glibc-devel / libc6-dev install is incomplete.")
	endif()
endif()
