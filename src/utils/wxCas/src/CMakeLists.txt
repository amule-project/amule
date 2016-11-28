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
	SET_TARGET_PROPERTIES (wxcas
		PROPERTIES WIN32_EXECUTABLE TRUE
	)
ENDIF (WIN32)

TARGET_COMPILE_DEFINITIONS (wxcas
	PRIVATE ${wxWidgets_NET_DEFS}
	PRIVATE ${wxWidgets_GUI_DEFS}
)

TARGET_LINK_LIBRARIES (wxcas
	${wxWidgets_GUI_LIBRARIES}
	${wxWidgets_NET_LIBRARIES}
)

INSTALL (TARGETS wxcas
	RUNTIME DESTINATION bin
)
