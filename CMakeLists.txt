cmake_minimum_required (VERSION 3.13)

# project
project(codec-share)

# codec target
add_library(codec-share
	./src/coder.cpp
	./src/token.cpp
)

# includes
target_include_directories(codec-share
PUBLIC
	./include
)

# test
enable_testing()
add_subdirectory(test)
