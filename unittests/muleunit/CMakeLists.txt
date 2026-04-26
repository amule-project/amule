add_library (muleunit OBJECT
	test.cpp
	main.cpp
	testcase.cpp
	testregistry.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/MuleDebug.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
)

target_compile_definitions (muleunit
	PUBLIC MULEUNIT
	PUBLIC wxUSE_GUI=0
)

target_include_directories (muleunit
	PUBLIC ${amule_BINARY_DIR}
	PUBLIC ${CMAKE_SOURCE_DIR}/src/libs
	INTERFACE ${CMAKE_SOURCE_DIR}/unittests
)

target_link_libraries (muleunit
	wxWidgets::BASE
)

if (HAVE_BFD)
	target_link_libraries (muleunit
		${BFD_LIBRARY}
	)
endif()
