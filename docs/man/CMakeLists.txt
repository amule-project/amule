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

# Translator-only target: regenerate manpages-{lang}.po and translated *.{lang}.1
# files from the English masters. po4a is not a build dependency; this target
# only exists when the tool is found on the host. Run it explicitly:
#     cmake --build build --target po4a-update
find_program (PO4A_EXECUTABLE po4a)
if (PO4A_EXECUTABLE)
	add_custom_target (po4a-update
		COMMAND ${PO4A_EXECUTABLE} po4a.config
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Refreshing translated manpages via po4a"
		VERBATIM
	)
endif()
