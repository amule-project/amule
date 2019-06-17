ADD_LIBRARY (libmuleunit STATIC
	test.cpp
	main.cpp
	testcase.cpp
	testregistry.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/MuleDebug.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
)

TARGET_COMPILE_DEFINITIONS (libmuleunit
	PRIVATE "-DMULEUNIT" ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (libmuleunit
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
)
