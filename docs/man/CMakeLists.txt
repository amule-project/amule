if (BUILD_AMULECMD)
	check_manpage ("amulecmd")
endif()

if (BUILD_DAEMON)
	check_manpage ("amuled")
endif()

if (BUILD_ED2K)
	check_manpage ("ed2k")
endif()

if (BUILD_MONOLITHIC)
	check_manpage ("amule")
endif()

if (BUILD_REMOTEGUI)
	check_manpage ("amulegui")
endif()

if (BUILD_WEBSERVER)
	check_manpage ("amuleweb")
endif()
