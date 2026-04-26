if (NOT YYENABLE_NLS)
	include (CheckIncludeFile)

	check_include_file (argz.h HAVE_ARGZ_H)

	if (NOT HAVE_ARGZ_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (limits.h HAVE_LIMITS_H)

	if (NOT HAVE_LIMITS_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (locale.h HAVE_LOCALE_H)

	if (NOT HAVE_LOCALE_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (nl_types.h HAVE_NL_TYPES_H)

	if (NOT HAVE_NL_TYPES_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (stdlib.h HAVE_STDLIB_H)

	if (NOT HAVE_STDLIB_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (string.h HAVE_STRING_H)

	if (NOT HAVE_STRING_H)
		set (ENABLE_NLS FALSE)
	endif()

	check_include_file (sys/param.h HAVE_SYS_PARAM_H)

	if (NOT HAVE_SYS_PARAM_H)
		set (ENABLE_NLS FALSE)
	endif()

	if (NOT HAVE_STDDEF_H)
		check_include_file (stddef.h HAVE_STDDEF_H)

		if (NOT HAVE_STDDEF_H)
			set (ENABLE_NLS FALSE)
		endif()
	endif()

	include (FindGettext)

	if (NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT GETTEXT_MSGMERGE_EXECUTABLE)
		set (ENABLE_NLS FALSE)
	endif()

	if (ENABLE_NLS)
		message (STATUS "Everything is fine. aMule can be localized")
		set (YYENABLE_NLS TRUE CACHE INTERNAL "For parser, php-parser and to not recheck for nls-support" FORCE)
	else()
		message (STATUS "You need to install GNU gettext/gettext-tools to compile aMule with i18n support.")
	endif()
endif()
