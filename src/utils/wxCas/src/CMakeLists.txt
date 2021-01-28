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
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)

	set_target_properties (wxcas
		PROPERTIES WIN32_EXECUTABLE TRUE
	)
endif()

target_link_libraries (wxcas
	wxWidgets::CORE
	wxWidgets::NET
)

install (TARGETS wxcas
	RUNTIME DESTINATION bin
)
