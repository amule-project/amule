INCLUDE (${CMAKE_SOURCE_DIR}/cmake/source-vars.cmake)

IF (BISON_FOUND)
	BISON_TARGET(php_parser.c php_parser.y
		${CMAKE_CURRENT_BINARY_DIR}/php_parser.c
		COMPILE_FLAGS "-t -v -p php"
	)
ENDIF (BISON_FOUND)

IF (FLEX_FOUND)
	FLEX_TARGET(php_lexer.c php_lexer.l
		${CMAKE_CURRENT_BINARY_DIR}/php_lexer.c
		COMPILE_FLAGS "-P php"
	)
ENDIF (FLEX_FOUND)

ADD_EXECUTABLE (amuleweb
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

TARGET_COMPILE_DEFINITIONS (amuleweb
	PRIVATE ${wxWidgets_BASE_DEFS}
	PRIVATE ${wxWidgets_NET_DEFS}
)

SET_TARGET_PROPERTIES (amuleweb
	PROPERTIES COMPILE_FLAGS "-I${EC_INCLUDE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} -I${LIBS_INCLUDE_DIR} -I${SRC_DIR} -I${INCLUDE_INCLUDE_DIR} ${UPNP_FLAGS}"
)

TARGET_LINK_LIBRARIES (amuleweb
	ec
	mulecommon
	mulesocket
	${wxWidgets_NET_LIBRARIES}
	${wxWidgets_BASE_LIBRARIES}
	${ZLIB_LIBRARIES}
)

IF (ENABLE_UPNP)
	TARGET_LINK_LIBRARIES (amuleweb ${LIBUPNP_LIBRARIES})
ENDIF (ENABLE_UPNP)

IF (HAVE_BFD)
	TARGET_LINK_LIBRARIES (amuleweb ${LIBBFD})
ENDIF (HAVE_BFD)

IF (HAVE_LIBREADLINE)
	TARGET_LINK_LIBRARIES (amuleweb ${READLINE_LIBRARIES})
ENDIF (HAVE_LIBREADLINE)

IF (WITH_LIBPNG)
	TARGET_LINK_LIBRARIES (amuleweb ${PNG_LIBRARIES})
ENDIF (WITH_LIBPNG)

install(TARGETS amuleweb
            RUNTIME DESTINATION bin
)
