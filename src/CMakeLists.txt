SET (LIBS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
SET (INCLUDE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
SET (SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET (EC_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp")


ADD_CUSTOM_COMMAND (
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
)

ADD_CUSTOM_COMMAND (
	OUTPUT ${EC_INCLUDE_DIR}/ECCodes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECCodes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

ADD_CUSTOM_COMMAND (
	OUTPUT ${EC_INCLUDE_DIR}/ECTagTypes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECTagTypes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

ADD_CUSTOM_TARGET (generate_CountryFlags.h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h)
ADD_CUSTOM_TARGET (generate_ECCodes.h DEPENDS ${EC_INCLUDE_DIR}/ECCodes.h)
ADD_CUSTOM_TARGET (generate_ECTagTypes.h DEPENDS ${EC_INCLUDE_DIR}/ECTagTypes.h)
SET_SOURCE_FILES_PROPERTIES (${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h PROPERTIES GENERATED TRUE)
SET_SOURCE_FILES_PROPERTIES (${EC_INCLUDE_DIR}/ECCodes.h PROPERTIES GENERATED TRUE)
SET_SOURCE_FILES_PROPERTIES (${EC_INCLUDE_DIR}/ECTagTypes.h PROPERTIES GENERATED TRUE)

IF (BUILD_UTIL)
	ADD_SUBDIRECTORY (utils)
ENDIF (BUILD_UTIL)

IF (BUILD_WEBSERVER)
	ADD_SUBDIRECTORY (webserver)
ENDIF (BUILD_WEBSERVER)

IF (INSTALL_SKINS)
	ADD_SUBDIRECTORY (skins)
ENDIF (INSTALL_SKINS)

INCLUDE (${CMAKE_SOURCE_DIR}/cmake/source-vars.cmake)

IF (ENABLE_IP2COUNTRY)
	SET_SOURCE_FILES_PROPERTIES (${IP2COUNTRY}
		PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
	)
ENDIF (ENABLE_IP2COUNTRY)

IF (NEED_LIB)
	ADD_SUBDIRECTORY (libs)
ENDIF (NEED_LIB)

IF (BUILD_AMULECMD)
	ADD_EXECUTABLE (amulecmd
		DataToText.cpp
		ExternalConnector.cpp
		LoggerConsole.cpp
		NetworkFunctions.cpp
		OtherFunctions.cpp
		TextClient.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (amulecmd
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (amulecmd
		PRIVATE ${wxWidgets_BASE_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (amulecmd
		PRIVATE ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${SRC_DIR}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	TARGET_LINK_LIBRARIES (amulecmd
		mulecommon
		mulesocket
		ec
		${ZLIB_LIBRARIES}
	)

	IF (ENABLE_BOOST)
		TARGET_LINK_LIBRARIES (amulecmd
			boost_system
		)
		TARGET_COMPILE_DEFINITIONS (amulecmd
		PRIVATE ${wxWidgets_NET_DEFS}
	)
	ELSE (ENABLE_BOOST)
		TARGET_LINK_LIBRARIES (amulecmd
			${wxWidgets_NET_LIBRARIES}
		)
	ENDIF (ENABLE_BOOST)

	IF (HAVE_BFD)
		TARGET_LINK_LIBRARIES (amulecmd ${LIBBFD})
	ENDIF (HAVE_BFD)

	IF (HAVE_LIBREADLINE)
		TARGET_LINK_LIBRARIES (amulecmd ${READLINE_LIBRARIES})
	ENDIF (HAVE_LIBREADLINE)

	LIST (APPEND INSTALL_TARGETS "amulecmd")
ENDIF (BUILD_AMULECMD)

IF (BUILD_DAEMON)
	ADD_EXECUTABLE (amuled
		${CORE_SOURCES}
		${COMMON_SOURCES}
		amuled.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (amuled
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (amuled
		PRIVATE "AMULE_DAEMON"
		PRIVATE ${wxWidgets_BASE_DEFS}
		PRIVATE ${wxWidgets_NET_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (amuled
		PRIVATE ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${LIBUPNP_INCLUDE_DIRS}
		PRIVATE ${SRC_DIR}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	TARGET_LINK_LIBRARIES (amuled
		ec
		muleappcommon
		muleappcore
		mulecommon
		mulesocket
		${wxWidgets_NET_LIBRARIES}
		${ZLIB_LIBRARIES}
	)

	IF (ENABLE_BOOST)
		TARGET_LINK_LIBRARIES (amuled
			boost_system
		)
	ENDIF (ENABLE_BOOST)

	IF (ENABLE_UPNP)
		TARGET_LINK_LIBRARIES (amuled ${LIBUPNP_LIBRARIES})
	ENDIF (ENABLE_UPNP)

	IF (HAVE_BFD)
		TARGET_LINK_LIBRARIES (amuled ${LIBBFD})
	ENDIF (HAVE_BFD)

	IF (WIN32)
		TARGET_LINK_LIBRARIES (amuled
			shlwapi.lib
		)
	ENDIF (WIN32)

	LIST (APPEND INSTALL_TARGETS "amuled")
ENDIF (BUILD_DAEMON)

IF (BUILD_ED2K)
	ADD_EXECUTABLE (ed2k
		ED2KLinkParser.cpp
		MagnetURI.cpp
		MuleCollection.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (ed2k
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (ed2k
		PRIVATE "USE_STD_STRING"
	)

	IF (WIN32)
		TARGET_LINK_LIBRARIES (ed2k
			shlwapi.lib
		)
	ENDIF (WIN32)

	LIST (APPEND INSTALL_TARGETS "ed2k")
ENDIF (BUILD_ED2K)

IF (BUILD_MONOLITHIC)
	ADD_EXECUTABLE (amule
		${COMMON_SOURCES}
		${CORE_SOURCES}
		${GUI_SOURCES}
		CaptchaDialog.cpp
		CaptchaGenerator.cpp
		PartFileConvert.cpp
		PartFileConvertDlg.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (amule
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (amule
		PRIVATE ${wxWidgets_GUI_DEFS}
		PRIVATE ${wxWidgets_NET_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (amule
		PRIVATE ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${LIBUPNP_INCLUDE_DIRS}
		PRIVATE ${wxWidgets_ADV_DEFS}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	TARGET_LINK_LIBRARIES (amule
		ec
		muleappcommon
		muleappcore
		muleappgui
		mulecommon
		mulesocket
		${wxWidgets_ADV_LIBRARIES}
		${wxWidgets_GUI_LIBRARIES}
		${wxWidgets_NET_LIBRARIES}
		${wxWidgets_BASE_LIBRARIES}
		${ZLIB_LIBRARIES}
	)

	IF (ENABLE_BOOST)
		TARGET_LINK_LIBRARIES (amule
			boost_system
		)
	ENDIF (ENABLE_BOOST)

	IF (ENABLE_UPNP)
		TARGET_LINK_LIBRARIES (amule ${LIBUPNP_LIBRARIES})
	ENDIF (ENABLE_UPNP)

	IF (ENABLE_IP2COUNTRY)
		TARGET_COMPILE_DEFINITIONS (amule
			PRIVATE ENABLE_IP2COUNTRY
		)

		IF (GEOIP_INCLUDE_DIR)
			 TARGET_INCLUDE_DIRECTORIES (amule
		                PRIVATE ${GEOIP_INCLUDE_DIR}
			)
		ENDIF (GEOIP_INCLUDE_DIR)

		TARGET_LINK_LIBRARIES (amule ${LIBGEOIP})
	ENDIF (ENABLE_IP2COUNTRY)

	IF (HAVE_BFD)
		TARGET_LINK_LIBRARIES (amule ${LIBBFD})
	ENDIF (HAVE_BFD)

	IF (WIN32)
		TARGET_LINK_LIBRARIES (amule
			shlwapi.lib
		)

		SET_TARGET_PROPERTIES (amule
			PROPERTIES WIN32_EXECUTABLE TRUE
		)
	ENDIF (WIN32)

	LIST (APPEND INSTALL_TARGETS "amule")
ENDIF (BUILD_MONOLITHIC)

IF (BUILD_REMOTEGUI)
	ADD_EXECUTABLE (amulegui
		${COMMON_SOURCES}
		${GUI_SOURCES}
		kademlia/utils/UInt128.cpp
		amule-remote-gui.cpp
	)

	IF (WIN32)
		TARGET_SOURCES (amulegui
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	ENDIF (WIN32)

	TARGET_COMPILE_DEFINITIONS (amulegui
		PRIVATE "CLIENT_GUI"
		PRIVATE ${wxWidgets_GUI_DEFS}
		PRIVATE ${wxWidgets_NET_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (amulegui
		PRIVATE ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${SRC_DIR}
		PRIVATE ${wxWidgets_ADV_DEFS}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	TARGET_LINK_LIBRARIES (amulegui
		ec
		muleappcommon
		muleappgui
		mulecommon
		mulesocket
		${wxWidgets_ADV_LIBRARIES}
		${wxWidgets_GUI_LIBRARIES}
		${wxWidgets_NET_LIBRARIES}
		${ZLIB_LIBRARIES}
	)

	IF (ENABLE_BOOST)
		TARGET_LINK_LIBRARIES (amulegui
			boost_system
		)
	ENDIF (ENABLE_BOOST)

	IF (ENABLE_IP2COUNTRY)
		TARGET_COMPILE_DEFINITIONS (amulegui
			PRIVATE ENABLE_IP2COUNTRY
		)

		TARGET_LINK_LIBRARIES (amulegui ${LIBGEOIP})
	ENDIF (ENABLE_IP2COUNTRY)

	IF (HAVE_BFD)
		TARGET_LINK_LIBRARIES (amulegui ${LIBBFD})
	ENDIF (HAVE_BFD)

	IF (WIN32)
		SET_TARGET_PROPERTIES (amulegui
			PROPERTIES WIN32_EXECUTABLE TRUE
		)
	ENDIF (WIN32)

	LIST (APPEND INSTALL_TARGETS "amulegui")
ENDIF (BUILD_REMOTEGUI)

IF (NEED_LIB_MULEAPPCOMMON)
	ADD_LIBRARY (muleappcommon STATIC
		${UPNP_SOURCES}
		CFile.cpp
		ClientCredits.cpp
		DataToText.cpp
		ED2KLink.cpp
		Friend.cpp
		GapList.cpp
		MagnetURI.cpp
		MemFile.cpp
		NetworkFunctions.cpp
		OtherFunctions.cpp
		Packet.cpp
		RLE.cpp
		SafeFile.cpp
		SHA.cpp
		Tag.cpp
		TerminationProcess.cpp
		Timer.cpp
	)

	TARGET_COMPILE_DEFINITIONS (muleappcommon
		PRIVATE ${wxWidgets_BASE_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (muleappcommon
		PRIVATE ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBUPNP_INCLUDE_DIRS}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	ADD_DEPENDENCIES (muleappcommon
		generate_ECCodes.h
		generate_ECTagTypes.h
	)
ENDIF (NEED_LIB_MULEAPPCOMMON)

IF (NEED_LIB_MULEAPPCORE)
	IF (BISON_FOUND)
		BISON_TARGET (Parser.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Parser.y
			${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp
			COMPILE_FLAGS "-t -d -v"
		)

		SET (PARSER ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
	ELSE (BISON_FOUND)
		SET (PARSER ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp)
	ENDIF (BISON_FOUND)

	IF (FLEX_FOUND)
		IF (FLEX_MATCH)
			SET (FLEX_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.h")
			SET_SOURCE_FILES_PROPERTIES (Parser.cpp
				COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}"
			)
		ENDIF (FLEX_MATCH)

		FLEX_TARGET (Scanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Scanner.l
			${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp
			COMPILE_FLAGS "${FLEX_FLAGS}"
		)

		FLEX_TARGET (IPFilterScanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.l
			${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp
			COMPILE_FLAGS "-Pyyip"
		)

		SET (SCANNER ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp)
		SET (IPFILTERSCANNER ${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp)
	ELSE (FLEX_FOUND)
		SET (SCANNER ${CMAKE_CURRENT_SOURCE_DIR}/Scanner.cpp)
		SET (IPFILTERSCANNER ${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.cpp)
	ENDIF (FLEX_FOUND)

	ADD_LIBRARY (muleappcore STATIC
		${IPFILTERSCANNER}
		${PARSER}
		${SCANNER}
		kademlia/kademlia/Entry.cpp
		kademlia/kademlia/Indexed.cpp
		kademlia/kademlia/SearchManager.cpp
		kademlia/routing/RoutingBin.cpp
		kademlia/utils/UInt128.cpp
		AsyncDNS.cpp
		CanceledFileList.cpp
		DeadSourceList.cpp
		FileArea.cpp
		FileAutoClose.cpp
		PlatformSpecific.cpp
		RandomFunctions.cpp
		RC4Encrypt.cpp
		StateMachine.cpp
		TerminationProcessAmuleweb.cpp
		ThreadScheduler.cpp
		UPnPBase.cpp
	)

	TARGET_COMPILE_DEFINITIONS (muleappcore
		PRIVATE ${wxWidgets_BASE_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (muleappcore
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBUPNP_INCLUDE_DIRS}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)
ENDIF (NEED_LIB_MULEAPPCORE)

IF (NEED_LIB_MULEAPPGUI)
	ADD_LIBRARY (muleappgui STATIC
		${IP2COUNTRY}
		extern/wxWidgets/listctrl.cpp
		BarShader.cpp
		ColorFrameCtrl.cpp
		EditServerListDlg.cpp
		FileDetailListCtrl.cpp
		MuleCollection.cpp
		MuleColour.cpp
		MuleGifCtrl.cpp
		MuleListCtrl.cpp
		MuleNotebook.cpp
		MuleTextCtrl.cpp
		muuli_wdr.cpp
	)

	IF (IP2COUNTRY AND GEOIP_INCLUDE_DIR)
		 TARGET_INCLUDE_DIRECTORIES (muleappgui
	                PRIVATE ${GEOIP_INCLUDE_DIR}
		)
	ENDIF (IP2COUNTRY AND GEOIP_INCLUDE_DIR)

	TARGET_COMPILE_DEFINITIONS (muleappgui
		PRIVATE ${wxWidgets_GUI_DEFS}
	)

	TARGET_INCLUDE_DIRECTORIES (muleappgui
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)
ENDIF (NEED_LIB_MULEAPPGUI)

IF (ENABLE_IP2COUNTRY)
	ADD_DEPENDENCIES (muleappgui generate_CountryFlags.h)
ENDIF (ENABLE_IP2COUNTRY)

IF (NEED_LIB_MULESOCKET)
	ADD_LIBRARY (mulesocket STATIC
		LibSocket.cpp
	)

	TARGET_COMPILE_DEFINITIONS (mulesocket
		PRIVATE ${wxWidgets_BASE_DEFS}
	)
	
	TARGET_INCLUDE_DIRECTORIES (mulesocket
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)
ENDIF (NEED_LIB_MULESOCKET)

INSTALL (TARGETS ${INSTALL_TARGETS}
            RUNTIME DESTINATION bin
)