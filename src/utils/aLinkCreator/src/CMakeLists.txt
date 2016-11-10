IF (BUILD_ALCC)
	ADD_EXECUTABLE (alcc
		alcc.cpp
		ed2khash.cpp
		md4.cpp
	)

	TARGET_COMPILE_DEFINITIONS (alcc
		PRIVATE ${wxWidgets_BASE_DEFS}
	)

	TARGET_LINK_LIBRARIES (alcc ${wxWidgets_BASE_LIBRARIES})

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

	TARGET_COMPILE_DEFINITIONS (alc
		PRIVATE ${wxWidgets_GUI_DEFS}
	)

	TARGET_LINK_LIBRARIES (alc ${wxWidgets_GUI_LIBRARIES})

	INSTALL (TARGETS alc
            RUNTIME DESTINATION bin
	)
ENDIF (BUILD_ALC)
