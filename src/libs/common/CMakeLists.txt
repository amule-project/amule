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

target_link_libraries (mulecommon
	wxWidgets::BASE
)

if (HAVE_BFD)
	target_link_libraries (mulecommon
		${BFD_LIBRARY}
	)
endif()
