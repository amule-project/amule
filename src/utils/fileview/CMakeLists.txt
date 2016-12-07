ADD_EXECUTABLE (fileview
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

IF (WIN32)
	TARGET_SOURCES (fileview
		PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
	)
ENDIF (WIN32)

TARGET_COMPILE_DEFINITIONS (fileview
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (fileview
	PRIVATE ${INCLUDE_INCLUDE_DIR}
	PRIVATE ${LIBS_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES (fileview
	mulecommon
	${CRYPTOLIB}
)

INSTALL (TARGETS fileview
	RUNTIME DESTINATION bin
)
