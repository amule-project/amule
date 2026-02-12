# CMakeLists.txt for unified search tests

# Unit tests
add_executable(SearchIdTest
    unit/SearchIdTest.cpp
    ../core/SearchId.cpp
)

add_executable(SearchResultTest
    unit/SearchResultTest.cpp
)

add_executable(SearchParamsTest
    unit/SearchParamsTest.cpp
)

add_executable(SearchCommandTest
    unit/SearchCommandTest.cpp
)

add_executable(SearchEventTest
    unit/SearchEventTest.cpp
)

# Link against gtest
target_link_libraries(SearchIdTest PRIVATE gtest gtest_main)
target_link_libraries(SearchResultTest PRIVATE gtest gtest_main)
target_link_libraries(SearchParamsTest PRIVATE gtest gtest_main)
target_link_libraries(SearchCommandTest PRIVATE gtest gtest_main)
target_link_libraries(SearchEventTest PRIVATE gtest gtest_main)

# Enable testing
enable_testing()

# Add tests
add_test(NAME SearchIdTest COMMAND SearchIdTest)
add_test(NAME SearchResultTest COMMAND SearchResultTest)
add_test(NAME SearchParamsTest COMMAND SearchParamsTest)
add_test(NAME SearchCommandTest COMMAND SearchCommandTest)
add_test(NAME SearchEventTest COMMAND SearchEventTest)
