add_subdirectory (docs)

configure_file (
	autostart-xas.cm
	autostart-xas
)

install (FILES xas.pl
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/xchat/plugins/"
)

install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/autostart-xas
	DESTINATION "${CMAKE_INSTALL_BINDIR}/"
)
