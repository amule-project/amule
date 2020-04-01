ADD_LIBRARY (mulecommon STATIC
	FileFunctions.cpp
	Format.cpp
	MD5Sum.cpp
	MuleDebug.cpp
	Path.cpp
	strerror_r.c
	StringFunctions.cpp
	TextFile.cpp
)

TARGET_LINK_LIBRARIES (mulecommon
	wxWidgets::BASE
)

IF (HAVE_BFD)
	TARGET_LINK_LIBRARIES(mulecommon
		${BFD_LIBRARY}
	)
ENDIF (HAVE_BFD)
