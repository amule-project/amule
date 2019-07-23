ADD_LIBRARY (muleunit STATIC
	test.cpp
	main.cpp
	testcase.cpp
	testregistry.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/MuleDebug.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
)

TARGET_COMPILE_DEFINITIONS (muleunit
	PRIVATE "-DMULEUNIT" ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (muleunit
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE ${wxWidgets_INCLUDE_DIRS}
)

IF (HAVE_BFD)
	TARGET_LINK_LIBRARIES (muleunit ${LIBBFD})
ENDIF (HAVE_BFD)
