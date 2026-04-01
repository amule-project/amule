add_library (mulecommon STATIC
	FileFunctions.cpp
	Format.cpp
	MD5Sum.cpp
	MuleDebug.cpp
	Path.cpp
	strerror_r.c
	StringFunctions.cpp
	TextFile.cpp
	Logger.cpp
)

target_include_directories (mulecommon
	PRIVATE ${amule_BINARY_DIR}
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/libs
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries (mulecommon
	PUBLIC wxWidgets::BASE
)

if (HAVE_BFD)
	target_link_libraries (mulecommon
		PUBLIC ${BFD_LIBRARY}
	)
endif()
