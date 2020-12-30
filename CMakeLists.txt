cmake_minimum_required (VERSION 3.13)

# project
project(codec-share)

# properties
set(CMAKE_CXX_STANDARD 17)

# codec target
add_library(codec-share
	#./src/coder.cpp
	./src/token.cpp
	#./src/stream.cpp
)

# includes
target_include_directories(codec-share
PUBLIC
	./include
)

# test
enable_testing()
add_subdirectory(test)
