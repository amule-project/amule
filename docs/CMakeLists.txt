add_subdirectory (man)

install (FILES
	amulesig.txt
	Changelog
	EC_Protocol.txt
	INSTALL
	IP2Country.md
	license.txt
	DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
