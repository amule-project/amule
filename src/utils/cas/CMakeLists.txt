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
		${CMAKE_SOURCE_DIR}/version.rc
	)
endif()

target_include_directories (cas
	PRIVATE ${CMAKE_BINARY_DIR}
)

target_link_libraries (cas
	PkgConfig::gdlib
)

install (TARGETS cas
	RUNTIME DESTINATION bin
)

install (FILES tmp.html stat.png
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cas"
)
