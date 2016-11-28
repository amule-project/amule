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

TARGET_COMPILE_DEFINITIONS (fileview
	PRIVATE ${wxWidgets_BASE_DEFS}
)

SET_TARGET_PROPERTIES (fileview
	PROPERTIES COMPILE_FLAGS "-I${INCLUDE_INCLUDE_DIR} -I${LIBS_INCLUDE_DIR}"
)

TARGET_LINK_LIBRARIES (fileview
	mulecommon
	${CRYPTOLIB}
)

INSTALL (TARGETS fileview
	RUNTIME DESTINATION bin
)
