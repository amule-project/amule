add_executable (wxcas
	linuxmon.cpp
	onlinesig.cpp
	wxcas.cpp
	wxcascte.cpp
	wxcasframe.cpp
	wxcasprefs.cpp
	wxcaspix.cpp
	wxcasprint.cpp
)

target_include_directories (wxcas
	PRIVATE ${amule_BINARY_DIR}
)

if (WIN32)
	target_sources (wxcas
		PRIVATE ${CMAKE_BINARY_DIR}/version.rc
	)

	set_target_properties (wxcas
		PROPERTIES WIN32_EXECUTABLE TRUE
	)
endif()

target_link_libraries (wxcas
	PRIVATE wxWidgets::CORE
	PRIVATE wxWidgets::NET
)

if (APPLE)
	target_link_libraries (wxcas
		PRIVATE "-framework CoreServices"
	)
endif()

install (TARGETS wxcas
	RUNTIME DESTINATION bin
)
