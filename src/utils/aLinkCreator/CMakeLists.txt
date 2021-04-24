add_subdirectory (docs)
add_subdirectory (src)

if (BUILD_ALC)
	install (FILES alc.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
	)

	install (FILES alc.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()
