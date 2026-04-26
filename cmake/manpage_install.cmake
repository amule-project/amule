function (CHECK_MANPAGE BINARY)
	file (GLOB MAN_PAGE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.1")

	foreach (MAN_PAGE ${MAN_PAGE_FILES})
		string (REGEX REPLACE "\\." ";" MAN_PAGE ${MAN_PAGE})
		list (LENGTH MAN_PAGE LENGTH)
		list (GET MAN_PAGE 0 NAME)

		if (NAME STREQUAL ${BINARY})
			if (LENGTH EQUAL 2)
				install (FILES "${NAME}.1"
					DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
				)
			else()
				if (ENABLE_NLS)
					list (GET MAN_PAGE 1 LANG)

					if (${TRANSLATION_${LANG}})
						install (FILES "${NAME}.${LANG}.1"
							DESTINATION "${CMAKE_INSTALL_MANDIR}/${LANG}/man1"
							RENAME "${NAME}.1"
						)
					endif()
				endif()
			endif()
		endif()
	endforeach()
endfunction()
