#include (${CMAKE_SOURCE_DIR}/cmake/source-vars.cmake)

if (BISON_FOUND)
	bison_target (php_parser.c php_parser.y
		${CMAKE_CURRENT_BINARY_DIR}/php_parser.c
		COMPILE_FLAGS "-t -v -p php"
	)
endif()

if (FLEX_FOUND)
	flex_target (php_lexer.c php_lexer.l
		${CMAKE_CURRENT_BINARY_DIR}/php_lexer.c
		COMPILE_FLAGS "-P php"
	)
endif()

add_executable (amuleweb
	${CMAKE_SOURCE_DIR}/src/ExternalConnector.cpp
	${CMAKE_SOURCE_DIR}/src/OtherFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/RLE.cpp
	${CMAKE_SOURCE_DIR}/src/NetworkFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/LoggerConsole.cpp
	${UPNP_SOURCES}
	php_amule_lib.cpp
	php_core_lib.cpp
	php_lexer.c
	php_parser.c
	php_syntree.cpp
	WebInterface.cpp
	WebServer.cpp
	WebSocket.cpp
)

if (WIN32)
	target_sources (amuleweb
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)
endif()

target_compile_definitions (amuleweb
	PRIVATE wxUSE_GUI=0
)

target_include_directories (amuleweb
	PRIVATE ${CMAKE_BINARY_DIR}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${LIBUPNP_INCLUDE_DIRS}
	PRIVATE ${SRC_DIR}
)

target_link_libraries (amuleweb
	PRIVATE ec
	PRIVATE mulecommon
	PRIVATE mulesocket
	PRIVATE wxWidgets::NET
)

if (ENABLE_UPNP)
	target_link_libraries (amuleweb
		PRIVATE UPNP::Shared
	)
endif()

if (HAVE_BFD)
	target_link_libraries (amuleweb
		PRIVATE ${LIBBFD}
	)
endif()

if (HAVE_LIBREADLINE)
	target_link_libraries (amuleweb
		PRIVATE ${READLINE_LIBRARIES}
	)
endif()

if (WITH_LIBPNG)
	target_include_directories (amuleweb
		PRIVATE ${PNG_INCLUDE_DIRS}
	)

	target_link_libraries (amuleweb
		PRIVATE ${PNG_LIBRARIES}
	)
endif()

install (TARGETS amuleweb
	RUNTIME DESTINATION bin
)
