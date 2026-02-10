add_library (ec STATIC
	ECMuleSocket.cpp
	ECPacket.cpp
	ECSpecialTags.cpp
	ECSocket.cpp
	ECTag.cpp
	ECUInt128.cpp
	RemoteConnect.cpp
)

add_dependencies (ec
	generate_ECCodes.h
	generate_ECTagTypes.h
)

target_compile_definitions (ec
	PRIVATE wxUSE_GUI=0
)

target_include_directories (ec
	PUBLIC ${CMAKE_BINARY_DIR}
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
	PRIVATE ${INCLUDE_INCLUDE_DIR}
	PRIVATE ${LIBS_INCLUDE_DIR}
)

if (ENABLE_BOOST)
	target_include_directories (ec
		PUBLIC ${Boost_INCLUDE_DIR}
	)
endif()

target_link_libraries (ec
	PRIVATE CRYPTOPP::CRYPTOPP
	PRIVATE wxWidgets::BASE
	PRIVATE ZLIB::ZLIB
)
