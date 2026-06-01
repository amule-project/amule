function (CHECK_MANPAGE BINARY)
	# `.1.in` files are templated via configure_file (@MAN_DATE@,
	# @PACKAGE_VERSION@ in the .TH header); plain `.1` files are installed
	# as-is. Both forms are supported in the same source tree so callers
	# can adopt templating incrementally.
	file (GLOB MAN_PAGE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.1" "*.1.in")

	foreach (MAN_PAGE ${MAN_PAGE_FILES})
		# Strip the optional `.in` tail before parsing components.
		string (REGEX REPLACE "\\.in$" "" MAN_BASE ${MAN_PAGE})
		string (REGEX REPLACE "\\." ";" MAN_TOKENS ${MAN_BASE})
		list (LENGTH MAN_TOKENS LENGTH)
		list (GET MAN_TOKENS 0 NAME)

		if (NOT NAME STREQUAL ${BINARY})
			continue()
		endif()

		# Determine the install-time filename and the source we read from.
		# For `.1.in`, configure_file emits to the binary dir; for `.1`,
		# install reads from the source dir directly.
		if (MAN_PAGE MATCHES "\\.in$")
			set (SRC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${MAN_BASE}")
			configure_file (
				"${CMAKE_CURRENT_SOURCE_DIR}/${MAN_PAGE}"
				"${SRC_FILE}"
				@ONLY
			)
		else()
			set (SRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${MAN_PAGE}")
		endif()

		if (LENGTH EQUAL 2)
			install (FILES "${SRC_FILE}"
				DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
			)
		else()
			if (ENABLE_NLS)
				list (GET MAN_TOKENS 1 LANG)

				if (${TRANSLATION_${LANG}})
					install (FILES "${SRC_FILE}"
						DESTINATION "${CMAKE_INSTALL_MANDIR}/${LANG}/man1"
						RENAME "${NAME}.1"
					)
				endif()
			endif()
		endif()
	endforeach()
endfunction()
