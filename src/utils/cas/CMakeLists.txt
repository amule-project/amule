add_subdirectory (docs)

add_executable (cas
	cas.c
	configfile.c
	functions.c
	graphics.c
	html.c
	lines.c
)

if (WIN32)
	target_sources (cas
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)
endif()

target_include_directories (cas
	PRIVATE ${CMAKE_BINARY_DIR}
)

target_link_libraries (cas
	PRIVATE PkgConfig::gdlib
)

if (APPLE)
	target_link_libraries (cas
		PRIVATE "-framework CoreServices"
	)
endif()

install (TARGETS cas
	RUNTIME DESTINATION bin
)

install (FILES tmp.html stat.png
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cas"
)
