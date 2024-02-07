include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY git@github.com:google/googletest.git
  GIT_TAG        f8d7d77 # v1.14
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(common_test common_test.cpp)
target_link_libraries(common_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(common_test)

