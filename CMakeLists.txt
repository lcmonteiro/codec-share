cmake_minimum_required (VERSION 3.16)
# -------------------------------------------------------------------
# project
# -------------------------------------------------------------------
project(codec-share)

# options
OPTION(ENABLE_TESTING    "Enable code testing support"   OFF)

# properties
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

# -------------------------------------------------------------------
# entry points
# -------------------------------------------------------------------
# codec library
add_library(codec-share
	./src/token.cpp
)
target_include_directories(codec-share
PUBLIC
	./include
)
# codec testing 
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

# -------------------------------------------------------------------
# summary
# -------------------------------------------------------------------
message(STATUS)
message(STATUS "${PROJECT_NAME} configuration:")
message(STATUS "  CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS "  ENABLE_TESTING   = ${ENABLE_TESTING}")
message(STATUS)

# -------------------------------------------------------------------
# end
# -------------------------------------------------------------------
