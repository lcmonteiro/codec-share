cmake_minimum_required (VERSION 3.13)

project(codec-share)

include(GoogleTest)

add_executable(tests tests.cpp)
target_link_libraries(tests GTest::GTest)
add_test(NAME monolithic COMMAND tests)

# codec target
add_library(codec-share
	./source/engine.cpp
	./source/token.cpp
)

# includes
target_include_directories(codec-share
PUBLIC
	./include
)

