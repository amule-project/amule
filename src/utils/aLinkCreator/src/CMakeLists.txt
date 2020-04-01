IF (BUILD_ALCC)
	ADD_EXECUTABLE (alcc
		alcc.cpp
		ed2khash.cpp
		md4.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (alcc
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (alcc
		PRIVATE wxUSE_GUI=0
	)

	TARGET_LINK_LIBRARIES (alcc
		wxWidgets::BASE
	)

	INSTALL (TARGETS alcc
            RUNTIME DESTINATION bin
	)
ENDIF (BUILD_ALCC)

IF (BUILD_ALC)
	ADD_EXECUTABLE (alc
		alc.cpp
		alcframe.cpp
		alcpix.cpp
		ed2khash.cpp
		md4.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (alc
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)

		SET_TARGET_PROPERTIES (alc
			PROPERTIES WIN32_EXECUTABLE TRUE
		)
	ENDIF (WIN32)

	TARGET_LINK_LIBRARIES (alc
		wxWidgets::CORE
	)

	INSTALL (TARGETS alc
            RUNTIME DESTINATION bin
	)
ENDIF (BUILD_ALC)
