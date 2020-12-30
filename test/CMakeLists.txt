cmake_minimum_required (VERSION 3.10)

# download gtest 
include(gtest.cmake)

# gtest function  
function(add_gtests TEST_TARGET)
	set(options)
	set(oneValue TARGET)
	set(multiValue INCLUDES SOURCES TEST_SOURCES MOCK_SOURCES DEPENDS DEFINITIONS)
	cmake_parse_arguments(ARG "${options}" "${oneValue}" "${multiValue}" ${ARGN})
	add_executable (
		${TEST_TARGET} ${ARG_SOURCES} ${ARG_TEST_SOURCES} ${ARG_MOCK_SOURCES} 
	)
	target_include_directories(${TEST_TARGET}
	PRIVATE
		${ARG_INCLUDES}
	)
	target_compile_definitions(${TEST_TARGET} 
	PRIVATE 
		${ARG_DEFINITIONS}
	)
	target_link_libraries(
		${TEST_TARGET}            
	PRIVATE
		GTest::GTest 
		GTest::Main
		${ARG_TARGET} 
		${ARG_DEPENDS}
	)
	add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endfunction()

# test
add_gtests(codec-share-test 
TARGET 
	codec-share 
TEST_SOURCES 
	./src/codec_share_test.cpp
	./src/codec_share_stream_test.cpp
	./src/codec_share_container_test.cpp
)

