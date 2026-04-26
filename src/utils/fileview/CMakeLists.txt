add_executable (fileview
	${SRC_DIR}/kademlia/utils/UInt128.cpp
	${SRC_DIR}/CFile.cpp
	${SRC_DIR}/OtherFunctions.cpp
	${SRC_DIR}/SafeFile.cpp
	${SRC_DIR}/Tag.cpp
	eD2kFiles.cpp
	FileView.cpp
	KadFiles.cpp
	Print.cpp
)

if (WIN32)
	target_sources (fileview
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)
endif()

target_compile_definitions (fileview
	PRIVATE wxUSE_GUI=0
)

target_include_directories (fileview
	PRIVATE ${CMAKE_BINARY_DIR}
	PRIVATE ${INCLUDE_INCLUDE_DIR}
	PRIVATE ${LIBS_INCLUDE_DIR}
)

target_link_libraries (fileview
	PRIVATE mulecommon
	PRIVATE CRYPTOPP::CRYPTOPP
)

install (TARGETS fileview
	RUNTIME DESTINATION bin
)
