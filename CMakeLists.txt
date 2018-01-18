
cmake_minimum_required(VERSION 2.8)
project( contour )
find_package( OpenCV REQUIRED )

add_compile_options(-g -std=c++11)

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MY_COMPILE_FLAGS}" )

add_executable( contour main.cpp )
target_link_libraries( contour ${OpenCV_LIBS} tbb)

