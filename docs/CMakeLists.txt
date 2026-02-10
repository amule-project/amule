add_subdirectory (man)

install (FILES
	ABOUT-NLS
	amulesig.txt
	Changelog
	EC_Protocol.txt
	INSTALL
	license.txt
	README
	TODO
	DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
