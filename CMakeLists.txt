cmake_minimum_required(VERSION 3.13)

project (VideoSubFinderWXW)

if (WIN32)
	option(USE_CUDA "Build with CUDA support" ON)	
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
	option(USE_CUDA "Build with CUDA support" OFF)
	find_package(wxWidgets REQUIRED COMPONENTS core base aui)
	find_package(OpenCV REQUIRED)
	# find_package(PkgConfig REQUIRED)
	# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
		# libavcodec
	# )
endif (WIN32)

if (USE_CUDA)
	add_definitions(-DUSE_CUDAKernels)
	set(CUDAKernels_INCLUDE_DIRS "../CUDAKernels")
else()
	set(CUDAKernels_INCLUDE_DIRS "")	
endif(USE_CUDA)

add_subdirectory(Components/IPAlgorithms)
add_subdirectory(Components/OCVVideo)
add_subdirectory(Components/FFMPEGVideo)
add_subdirectory(Interfaces/VideoSubFinderWXW)

if (WIN32)
	# if(MSVC)
		# set_property(TARGET IPAlgorithms PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
		# set_property(TARGET OCVVideo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
		# set_property(TARGET FFMPEGVideo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
		# set_property(TARGET VideoSubFinderWXW PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

		# set_property(TARGET IPAlgorithms PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
		# set_property(TARGET OCVVideo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
		# set_property(TARGET FFMPEGVideo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
		# set_property(TARGET VideoSubFinderWXW PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
	# endif(MSVC)
endif(WIN32)

