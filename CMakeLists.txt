cmake_minimum_required(VERSION 3.13)

option(USE_CUDA "Build with CUDA support" ON)

if (USE_CUDA)
	project(VideoSubFinderWXW LANGUAGES CXX CUDA)
else()
	project(VideoSubFinderWXW LANGUAGES CXX)
endif(USE_CUDA)

if (WIN32)
	set(wxWidgets_INCLUDE_DIRS "$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib/mswu;$ENV{WX_WIDGETS_PATH}/include" CACHE STRING "Paths to OpenCV include dirs")
	set(OpenCV_INCLUDE_DIRS "$ENV{OPENCV_PATH_X64}/builds/Win64/install/include" CACHE STRING "Paths to OpenCV include dirs")
	set(FFMPEG_INCLUDE_DIRS "$ENV{FFMPEG_PATH_X64}/include" CACHE STRING "Paths to FFMPEG include dirs")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")	
	set(CompilerFlags
			CMAKE_CXX_FLAGS
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			)
	foreach(CompilerFlag ${CompilerFlags})
	  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
else()
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(wxWidgets_USE_DEBUG ON)
	else()
	set(wxWidgets_USE_DEBUG OFF)
	endif()	
	find_package(wxWidgets REQUIRED COMPONENTS core base aui)
	find_package(OpenCV REQUIRED)
endif (WIN32)

if (USE_CUDA)
	set(CMAKE_CUDA_FLAGS "-arch=all")
	add_definitions(-DUSE_CUDA)
	set(CUDAKernels_INCLUDE_DIRS "../CUDAKernels")
	set(CUDAKernels_LIB "CUDAKernels;cudart;nppicc;nppig")
	add_subdirectory(Components/CUDAKernels)
else()
	set(CUDAKernels_INCLUDE_DIRS "")
	set(CUDAKernels_LIB "")
endif(USE_CUDA)

add_subdirectory(Components/IPAlgorithms)
add_subdirectory(Components/OCVVideo)
add_subdirectory(Components/FFMPEGVideo)
add_subdirectory(Interfaces/VideoSubFinderWXW)

