set (LIBS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set (INCLUDE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set (SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (EC_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp")

add_custom_command (
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
)

add_custom_command (
	OUTPUT ${EC_INCLUDE_DIR}/ECCodes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECCodes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

add_custom_command (
	OUTPUT ${EC_INCLUDE_DIR}/ECTagTypes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECTagTypes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

add_custom_target (generate_CountryFlags.h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h)
add_custom_target (generate_ECCodes.h DEPENDS ${EC_INCLUDE_DIR}/ECCodes.h)
add_custom_target (generate_ECTagTypes.h DEPENDS ${EC_INCLUDE_DIR}/ECTagTypes.h)
set_source_files_properties (${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h PROPERTIES GENERATED TRUE)
set_source_files_properties (${EC_INCLUDE_DIR}/ECCodes.h PROPERTIES GENERATED TRUE)
set_source_files_properties (${EC_INCLUDE_DIR}/ECTagTypes.h PROPERTIES GENERATED TRUE)
include (${CMAKE_SOURCE_DIR}/cmake/source-vars.cmake)

if (BUILD_UTIL)
	add_subdirectory (utils)
endif()

if (BUILD_WEBSERVER)
	add_subdirectory (webserver)
endif()

if (INSTALL_SKINS)
	add_subdirectory (skins)
endif()


if (ENABLE_IP2COUNTRY)
	set_source_files_properties (${IP2COUNTRY}
		PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
	)
endif()

if (NEED_LIB)
	add_subdirectory (libs)
endif()

if (BUILD_AMULECMD)
	add_executable (amulecmd
		DataToText.cpp
		ExternalConnector.cpp
		LoggerConsole.cpp
		NetworkFunctions.cpp
		OtherFunctions.cpp
		TextClient.cpp
	)

	if (WIN32)
		target_sources (amulecmd
			PRIVATE ${CMAKE_BINARY_DIR}/version.rc
		)
	endif()

	target_compile_definitions (amulecmd
		PRIVATE wxUSE_GUI=0
	)

	target_include_directories (amulecmd
		PRIVATE ${SRC_DIR}
	)

	target_link_libraries (amulecmd
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE ec
	)

	if (HAVE_BFD)
		target_link_libraries (amulecmd 
			PRIVATE ${BFD_LIBRARY}
		)
	endif (HAVE_BFD)

	if (HAVE_LIBREADLINE)
		target_link_libraries (amulecmd
			PRIVATE ${READLINE_LIBRARIES}
		)
	endif()

	install (TARGETS amulecmd
		RUNTIME DESTINATION bin
	)
endif (BUILD_AMULECMD)

if (BUILD_DAEMON)
	add_executable (amuled
		${CORE_SOURCES}
		${COMMON_SOURCES}
		amuled.cpp
	)

	if (WIN32)
		target_sources (amuled
			PRIVATE ${CMAKE_BINARY_DIR}/version.rc
		)
	endif()

	target_compile_definitions (amuled
		PRIVATE AMULE_DAEMON
		PRIVATE wxUSE_GUI=0
	)

	target_include_directories (amuled
		PRIVATE ${SRC_DIR}
	)

	target_link_libraries (amuled
		PRIVATE ec
		PRIVATE muleappcommon
		PRIVATE muleappcore
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE wxWidgets::NET
	)

	if (GLIB_FOUND)
		# amule.cpp's CamuleApp::OnInit calls g_set_prgname; daemon
		# build pulls amule.cpp via CORE_SOURCES so it needs the
		# glib include path too.
		target_include_directories (amuled PRIVATE ${GLIB_INCLUDE_DIRS})
		target_link_libraries (amuled PRIVATE ${GLIB_LIBRARIES})
	endif()

	if (HAVE_BFD)
		target_link_libraries (amuled
			PRIVATE ${BFD_LIBRARY}
		)
	endif()

	if (WIN32)
		target_link_libraries (amuled
			PRIVATE shlwapi.lib
		)
	endif()

	install (TARGETS amuled
		RUNTIME DESTINATION bin
	)
endif()

if (BUILD_ED2K)
	add_executable (ed2k
		ED2KLinkParser.cpp
		MagnetURI.cpp
		MuleCollection.cpp
	)

	if (WIN32)
		target_sources (ed2k
			PRIVATE ${CMAKE_BINARY_DIR}/version.rc
		)
	endif()

	target_compile_definitions (ed2k
		PRIVATE "USE_STD_STRING"
	)

	if (WIN32)
		target_link_libraries (ed2k
			PRIVATE shlwapi.lib
		)
	endif()

	if (APPLE)
		target_link_libraries (ed2k
			PRIVATE "-framework CoreServices"
		)
	endif()

	install (TARGETS ed2k
		RUNTIME DESTINATION bin
	)
endif()

if (BUILD_MONOLITHIC)
	add_executable (amule
		${COMMON_SOURCES}
		${CORE_SOURCES}
		${GUI_SOURCES}
		CaptchaDialog.cpp
		CaptchaGenerator.cpp
		PartFileConvert.cpp
		PartFileConvertDlg.cpp
	)

	if (WIN32)
		target_sources (amule
			PRIVATE ${CMAKE_BINARY_DIR}/version.rc ${CMAKE_SOURCE_DIR}/amule.rc
		)
	endif()

	target_link_libraries (amule
		PRIVATE ec
		PRIVATE muleappcommon
		PRIVATE muleappcore
		PRIVATE muleappgui
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE $<$<VERSION_LESS:${wxWidgets_VERSION_STRING},3.1.2>:wxWidgets::ADV>
		PRIVATE wxWidgets::NET
	)

	if (HAVE_BFD)
		target_link_libraries (amule
			PRIVATE ${BFD_LIBRARY}
		)
	endif()

	if (GLIB_FOUND)
		# CamuleApp::OnInit calls g_set_prgname (Wayland app_id binding),
		# so amule.cpp needs the glib include path even when the
		# AppIndicator backend isn't compiled in.
		target_include_directories (amule PRIVATE ${GLIB_INCLUDE_DIRS})
		target_link_libraries (amule PRIVATE ${GLIB_LIBRARIES})
	endif()

	if (WITH_LIBAYATANA_APPINDICATOR)
		target_include_directories (amule PRIVATE ${AYATANA_APPINDICATOR_INCLUDE_DIRS})
		target_link_libraries (amule PRIVATE ${AYATANA_APPINDICATOR_LIBRARIES})
		target_compile_options (amule PRIVATE ${AYATANA_APPINDICATOR_CFLAGS_OTHER})
	endif()

	if (WIN32)
		target_link_libraries (amule
			PRIVATE shlwapi.lib
		)

		set_target_properties (amule PROPERTIES
			WIN32_EXECUTABLE TRUE
		)
	endif()

	if (APPLE)
		set_target_properties (amule PROPERTIES
			MACOSX_BUNDLE TRUE
			MACOSX_BUNDLE_BUNDLE_NAME "aMule"
			MACOSX_BUNDLE_GUI_IDENTIFIER "org.amule.aMule"
			MACOSX_BUNDLE_ICON_FILE "amule.icns"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
			MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
			MACOSX_BUNDLE_COPYRIGHT "Copyright 2003-2026 aMule Project"
			OUTPUT_NAME "aMule"
		)

		set (AMULE_ICNS "${CMAKE_SOURCE_DIR}/platforms/MacOSX/amule.icns")
		target_sources (amule PRIVATE ${AMULE_ICNS})
		set_source_files_properties (${AMULE_ICNS} PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources"
		)

		target_link_libraries (amule
			PRIVATE "-framework CoreServices"
			PRIVATE "-framework ApplicationServices"
		)

		# aMule performs HTTP downloads to user-configurable URLs
		# (server.met, IP filter, Kad nodes) and the upstream defaults for
		# some of these are plaintext http://. wxWebRequest is backed by
		# NSURLSession on macOS, which enforces App Transport Security and
		# blocks plaintext HTTP unless the bundle opts out. Inject the
		# opt-out into the CMake-generated Info.plist. This matches the
		# legacy wxHTTP/wxSocket path's behaviour, which bypassed ATS by
		# going through raw BSD sockets.
		add_custom_command (TARGET amule POST_BUILD
			COMMAND /usr/bin/plutil -replace NSAppTransportSecurity
				-json "{\"NSAllowsArbitraryLoads\": true}"
				"$<TARGET_BUNDLE_CONTENT_DIR:amule>/Info.plist"
			COMMENT "Opting aMule.app out of App Transport Security"
			VERBATIM
		)

		# Copy translation catalogs into the .app bundle so the in-tree
		# build picks them up.  wxLocale's lookup prefix on macOS is
		# wxStandardPaths::GetDataDir() + "/locale", which resolves to
		# Contents/Resources/locale inside the bundle.  Done here (not
		# in po/) because the `amule` target is defined in this file
		# while `po` is added earlier in the top-level CMakeLists.
		if (ENABLE_NLS AND AMULE_TRANSLATIONS)
			add_dependencies (amule pofiles)
			foreach (_lang IN LISTS AMULE_TRANSLATIONS)
				set (_mo_dir
					"$<TARGET_BUNDLE_CONTENT_DIR:amule>/Resources/locale/${_lang}/LC_MESSAGES")
				add_custom_command (TARGET amule POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E make_directory "${_mo_dir}"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
						"${CMAKE_BINARY_DIR}/po/${_lang}.gmo"
						"${_mo_dir}/amule.mo"
					COMMENT "Bundling ${_lang} catalog into aMule.app"
					VERBATIM
				)
			endforeach()
		endif()
	endif()

	install (TARGETS amule
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION .
	)

	# XPM fallback for Icon= lookups on systems too old to ship the
	# hicolor PNG path. Filename matches the .desktop's Icon= field.
	install (FILES aMule.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
		RENAME org.amule.aMule.xpm
	)
endif (BUILD_MONOLITHIC)

if (BUILD_REMOTEGUI)
	add_executable (amulegui
		${COMMON_SOURCES}
		${GUI_SOURCES}
		kademlia/utils/UInt128.cpp
		amule-remote-gui.cpp
	)

	if (WIN32)
		target_sources (amulegui
			PRIVATE ${CMAKE_BINARY_DIR}/version.rc ${CMAKE_SOURCE_DIR}/amule.rc
		)
	endif()

	target_compile_definitions (amulegui
		PRIVATE "CLIENT_GUI"
	)

	target_include_directories (amulegui
		PRIVATE ${SRC_DIR}
	)

	target_link_libraries (amulegui
		PRIVATE ec
		PRIVATE muleappcommon
		PRIVATE muleappgui
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE $<$<VERSION_LESS:${wxWidgets_VERSION_STRING},3.1.2>:wxWidgets::ADV>
		PRIVATE wxWidgets::NET
	)

	if (HAVE_BFD)
		target_link_libraries (amulegui
			PRIVATE ${BFD_LIBRARY}
		)
	endif()

	if (GLIB_FOUND)
		# amulegui pulls MuleTrayIcon.cpp via GUI_SOURCES; the SNI
		# backend includes app-indicator headers which transitively
		# need glib's include path.
		target_include_directories (amulegui PRIVATE ${GLIB_INCLUDE_DIRS})
		target_link_libraries (amulegui PRIVATE ${GLIB_LIBRARIES})
	endif()

	if (WITH_LIBAYATANA_APPINDICATOR)
		target_include_directories (amulegui PRIVATE ${AYATANA_APPINDICATOR_INCLUDE_DIRS})
		target_link_libraries (amulegui PRIVATE ${AYATANA_APPINDICATOR_LIBRARIES})
		target_compile_options (amulegui PRIVATE ${AYATANA_APPINDICATOR_CFLAGS_OTHER})
	endif()

	if (WIN32)
		set_target_properties (amulegui PROPERTIES
			WIN32_EXECUTABLE TRUE
		)
	endif()

	if (APPLE)
		# amulegui is a GUI wxApp (remote-control UI) — bundle it as a
		# .app so macOS gives it an icon in the Dock, a menu bar and
		# normal application lifecycle events, matching the monolithic
		# aMule.app. Shares the same .icns because it's the same visual
		# identity.
		set_target_properties (amulegui PROPERTIES
			MACOSX_BUNDLE TRUE
			MACOSX_BUNDLE_BUNDLE_NAME "aMuleGUI"
			MACOSX_BUNDLE_GUI_IDENTIFIER "org.amule.aMuleGUI"
			MACOSX_BUNDLE_ICON_FILE "amule.icns"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
			MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
			MACOSX_BUNDLE_COPYRIGHT "Copyright 2003-2026 aMule Project"
			OUTPUT_NAME "aMuleGUI"
		)

		set (AMULEGUI_ICNS "${CMAKE_SOURCE_DIR}/platforms/MacOSX/amule.icns")
		target_sources (amulegui PRIVATE ${AMULEGUI_ICNS})
		set_source_files_properties (${AMULEGUI_ICNS} PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources"
		)

		target_link_libraries (amulegui
			PRIVATE "-framework CoreServices"
			PRIVATE "-framework ApplicationServices"
		)

		# Same ATS opt-out as the monolithic amule.app: wxWebRequest on
		# macOS is backed by NSURLSession which blocks plaintext HTTP
		# by default; amulegui uses the same HTTP download path for its
		# own version checks via CHTTPDownloadThread.
		add_custom_command (TARGET amulegui POST_BUILD
			COMMAND /usr/bin/plutil -replace NSAppTransportSecurity
				-json "{\"NSAllowsArbitraryLoads\": true}"
				"$<TARGET_BUNDLE_CONTENT_DIR:amulegui>/Info.plist"
			COMMENT "Opting aMuleGUI.app out of App Transport Security"
			VERBATIM
		)
	endif()

	install (TARGETS amulegui
		RUNTIME DESTINATION bin
		BUNDLE DESTINATION .
	)
endif()

if (NEED_LIB_MULEAPPCOMMON)
	add_library (muleappcommon STATIC
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

	add_dependencies (muleappcommon
		generate_ECCodes.h
		generate_ECTagTypes.h
	)

	target_compile_definitions (muleappcommon
		PRIVATE wxUSE_GUI=0
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappcommon
		PUBLIC ${amule_BINARY_DIR}
		PUBLIC ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	target_link_libraries(muleappcommon
		PUBLIC wxWidgets::BASE
	)

	if (ENABLE_UPNP)
		target_link_libraries (muleappcommon
			PUBLIC UPNP::Shared
		)
	endif()
endif()

if (NEED_LIB_MULEAPPCORE)
	if (BISON_FOUND)
		bison_target (Parser.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Parser.y
			${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp
			COMPILE_FLAGS "-t -d -v"
		)

		set (PARSER ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
	else()
		set (PARSER ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp)
	endif (BISON_FOUND)

	if (FLEX_FOUND)
		if (FLEX_MATCH)
			set (FLEX_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.h")

			set_source_files_properties (Parser.cpp
				COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}"
			)
		endif()

		flex_target (Scanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Scanner.l
			${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp
			COMPILE_FLAGS "${FLEX_FLAGS}"
		)

		flex_target (IPFilterScanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.l
			${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp
			COMPILE_FLAGS "-Pyyip"
		)

		set (SCANNER ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp)
		set (IPFILTERSCANNER ${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp)
	else()
		set (SCANNER ${CMAKE_CURRENT_SOURCE_DIR}/Scanner.cpp)
		set (IPFILTERSCANNER ${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.cpp)
	endif()

	add_library (muleappcore STATIC
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

	target_compile_definitions (muleappcore
		PRIVATE wxUSE_GUI=0
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappcore
		PRIVATE ${CMAKE_BINARY_DIR}
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)

	if (ENABLE_UPNP)
		target_include_directories (muleappcore
			PRIVATE $<TARGET_PROPERTY:UPNP::Shared,INTERFACE_INCLUDE_DIRECTORIES>
		)
	endif()

	target_link_libraries (muleappcore
		PUBLIC wxWidgets::BASE
		PRIVATE CRYPTOPP::CRYPTOPP
	)

	# Parser.cpp's bison-generated yyparse() expands YY_(Msgid) to dgettext,
	# so libintl needs to be on the link line whenever NLS is enabled.
	# Empty/no-op on glibc; the .dylib / .dll.a on macOS / MinGW.  PUBLIC so
	# every consumer of muleappcore (amule, amuled, amulecmd, amulegui)
	# inherits the dependency.
	if (ENABLE_NLS AND Intl_FOUND)
		target_include_directories (muleappcore PUBLIC ${Intl_INCLUDE_DIRS})
		target_link_libraries (muleappcore PUBLIC ${Intl_LIBRARIES})
	endif()

	if (APPLE)
		target_link_libraries (muleappcore
			PRIVATE "-framework IOKit"
			PRIVATE "-framework CoreFoundation"
		)
	endif()
endif()

if (NEED_LIB_MULEAPPGUI)
	add_library (muleappgui STATIC
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

	if (ENABLE_IP2COUNTRY)
		target_link_libraries (muleappgui
			PUBLIC MaxMindDB::Shared
		)
	endif()

	target_compile_definitions (muleappgui
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappgui
		PUBLIC ${amule_BINARY_DIR}
		PUBLIC ${INCLUDE_INCLUDE_DIR}
		PUBLIC ${LIBS_INCLUDE_DIR}
	)

	target_link_libraries (muleappgui
		PRIVATE wxWidgets::CORE
	)

	if (ENABLE_IP2COUNTRY)
		add_dependencies (muleappgui
			generate_CountryFlags.h)
	endif()
endif()

IF (NEED_LIB_MULESOCKET)
	add_library (mulesocket STATIC
		LibSocket.cpp
	)

	target_compile_definitions (mulesocket
		PRIVATE wxUSE_GUI=0
	)
	
	target_include_directories (mulesocket
		PUBLIC ${amule_BINARY_DIR}
		PUBLIC ${INCLUDE_INCLUDE_DIR}
		PUBLIC ${LIBS_INCLUDE_DIR}
	)

	if (ENABLE_BOOST)
		target_link_libraries (mulesocket
			PRIVATE ${Boost_LIBRARIES}
			PUBLIC wxWidgets::BASE
		)
	else()
		target_link_libraries (mulesocket
			PUBLIC wxWidgets::NET
		)
	endif()
endif()

# On MinGW-built Windows targets, bundle the MSYS2 MinGW64 runtime DLLs
# next to the installed executables so the install tree is self-contained.
# Must live here (not in root CMakeLists.txt) because CMake appends
# subdirectory install scripts at the END of the root cmake_install.cmake,
# so a root-level install(CODE) would run before the exes are installed.
if (WIN32 AND MINGW AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
	get_filename_component (MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

	install (CODE "
		message (STATUS \"Resolving Windows runtime dependencies...\")
		file (GLOB _exes \"\${CMAKE_INSTALL_PREFIX}/bin/*.exe\")
		file (GET_RUNTIME_DEPENDENCIES
			EXECUTABLES \${_exes}
			RESOLVED_DEPENDENCIES_VAR _resolved
			UNRESOLVED_DEPENDENCIES_VAR _unresolved
			DIRECTORIES \"${MINGW_BIN_DIR}\"
			PRE_EXCLUDE_REGEXES
				\"api-ms-.*\"
				\"ext-ms-.*\"
			POST_EXCLUDE_REGEXES
				\".*[Ss]ystem32.*\"
				\".*SysWOW64.*\"
		)
		foreach (_dll \${_resolved})
			file (INSTALL \${_dll} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\" FOLLOW_SYMLINK_CHAIN)
		endforeach()
		list (LENGTH _resolved _count)
		message (STATUS \"Bundled \${_count} runtime DLL(s) into \${CMAKE_INSTALL_PREFIX}/bin\")
		if (_unresolved)
			message (WARNING \"Unresolved runtime dependencies:\\n  \${_unresolved}\")
		endif()
	")

	# MSYS2's libcurl is built with an absolute --with-ca-bundle path
	# (e.g. /clangarm64/etc/ssl/certs/ca-bundle.crt) that does not exist
	# on end-user machines, so wxWebRequest HTTPS fails with error 77
	# anywhere outside the dev box. Ship the MSYS2 CA bundle next to the
	# .exe so a portable install can find it; CamuleApp::OnInit points
	# CURL_CA_BUNDLE at it on startup when the user has not set their own.
	find_file (MINGW_CA_BUNDLE
		NAMES cert.pem ca-bundle.crt
		PATHS "${MINGW_BIN_DIR}/../etc/ssl/certs"
		      "${MINGW_BIN_DIR}/../etc/ssl"
		      "${MINGW_BIN_DIR}/../ssl/certs"
		      "${MINGW_BIN_DIR}/../ssl"
		NO_DEFAULT_PATH)
	if (MINGW_CA_BUNDLE)
		install (FILES "${MINGW_CA_BUNDLE}"
			DESTINATION bin
			RENAME ca-bundle.crt)
		message (STATUS "Will bundle CA certificates from ${MINGW_CA_BUNDLE}")
	else()
		message (WARNING
			"No MSYS2 CA bundle found under ${MINGW_BIN_DIR}/../etc/ssl; "
			"libcurl HTTPS will fail at runtime unless CURL_CA_BUNDLE is "
			"set externally. Install mingw-w64-*-ca-certificates and "
			"re-run cmake to populate this.")
	endif()
endif()
