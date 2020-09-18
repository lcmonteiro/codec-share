cmake_minimum_required (VERSION 3.8)

# codec target
add_library(share-codec
	./source/engine.cpp
	./source/stamp.cpp
)

# includes
target_include_directories(share-codec 
PUBLIC
	./include
)
