add_library (mulecommon STATIC
	FileFunctions.cpp
	Format.cpp
	MD5Sum.cpp
	MuleDebug.cpp
	Path.cpp
	strerror_r.c
	StringFunctions.cpp
	TextFile.cpp
)

target_include_directories (mulecommon
	PRIVATE ${amule_BINARY_DIR}
)

target_link_libraries (mulecommon
	PUBLIC wxWidgets::BASE
)

if (HAVE_BFD)
	target_link_libraries (mulecommon
		PUBLIC ${BFD_LIBRARY}
	)
endif()
