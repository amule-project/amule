ADD_EXECUTABLE (wxcas
	linuxmon.cpp
	onlinesig.cpp
	wxcas.cpp
	wxcascte.cpp
	wxcasframe.cpp
	wxcasprefs.cpp
	wxcaspix.cpp
	wxcasprint.cpp
)

IF (WIN32)
	TARGET_SOURCES (wxcas
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)

	SET_TARGET_PROPERTIES (wxcas
		PROPERTIES WIN32_EXECUTABLE TRUE
	)
ENDIF (WIN32)

TARGET_LINK_LIBRARIES (wxcas
	wxWidgets::CORE
	wxWidgets::NET
)

INSTALL (TARGETS wxcas
	RUNTIME DESTINATION bin
)
