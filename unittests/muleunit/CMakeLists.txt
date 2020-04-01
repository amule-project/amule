ADD_LIBRARY (muleunit OBJECT
	test.cpp
	main.cpp
	testcase.cpp
	testregistry.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/MuleDebug.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
)

TARGET_COMPILE_DEFINITIONS (muleunit
	PUBLIC MULEUNIT
	PUBLIC wxUSE_GUI=0
)

TARGET_INCLUDE_DIRECTORIES (muleunit
	PUBLIC ${CMAKE_SOURCE_DIR}/src/libs
	INTERFACE ${CMAKE_SOURCE_DIR}/unittests
)

TARGET_LINK_LIBRARIES (muleunit
	wxWidgets::BASE
)

IF (HAVE_BFD)
	TARGET_LINK_LIBRARIES (muleunit
		${BFD_LIBRARY}
	)
ENDIF (HAVE_BFD)
