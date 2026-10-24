cmake_minimum_required(VERSION 3.24)
file(STRINGS "Interfaces/VideoSubFinderWXW/MyResource.h" VSF_VERSION_LINE
    REGEX "^#define[ \t]+VSF_VERSION[ \t]+\"[0-9.]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" VSF_UPSTREAM_VERSION "${VSF_VERSION_LINE}")
if(NOT VSF_UPSTREAM_VERSION)
    message(FATAL_ERROR "Cannot read the upstream VideoSubFinder version")
endif()
project(VideoSubFinderCli VERSION ${VSF_UPSTREAM_VERSION} LANGUAGES CXX)

option(USE_CUDA "Enable CUDA image processing" OFF)
option(VSF_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(Threads REQUIRED)
configure_file(Settings/general.cfg ${CMAKE_BINARY_DIR}/settings/general.cfg COPYONLY)

add_library(vsf_core STATIC
    Components/IPAlgorithms/IPAlgorithms.cpp
    Components/IPAlgorithms/SSAlgorithms.cpp
    Components/IPAlgorithms/MyClosedFigure.cpp
    Components/IPAlgorithms/ImageCropper.cpp
    Components/OCVVideo/OCVVideo.cpp
    Components/OCVVideo/OCVVideoLoader.cpp)
target_compile_features(vsf_core PUBLIC cxx_std_17)
target_include_directories(vsf_core PUBLIC
    Components/Include Components/IPAlgorithms Components/OCVVideo
    3rdparty/json/include 3rdparty/thread-pool/include
    ${OpenCV_INCLUDE_DIRS})
target_link_libraries(vsf_core PUBLIC ${OpenCV_LIBS} Threads::Threads)
if(MSVC)
    target_compile_definitions(vsf_core PUBLIC WIN32 WIN64 NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_compile_options(vsf_core PUBLIC /utf-8 /EHsc)
endif()
if(USE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES "61;75;80;86;89;90")
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit 12 REQUIRED)
    add_subdirectory(Components/CUDAKernels)
    target_compile_definitions(vsf_core PUBLIC USE_CUDA)
    target_include_directories(vsf_core PUBLIC Components/CUDAKernels)
    target_link_libraries(vsf_core PUBLIC CUDAKernels CUDA::cudart CUDA::nppicc CUDA::nppig)
endif()
if(VSF_SANITIZERS AND NOT MSVC)
    target_compile_options(vsf_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(vsf_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(VideoSubFinderCli Interfaces/VideoSubFinderCli/VideoSubFinderCli.cpp)
target_link_libraries(VideoSubFinderCli PRIVATE vsf_core)
target_compile_definitions(VideoSubFinderCli PRIVATE VSF_VERSION="${PROJECT_VERSION}")
set_target_properties(VideoSubFinderCli PROPERTIES
    BUILD_RPATH_USE_ORIGIN ON INSTALL_RPATH "$ORIGIN/lib;@loader_path/lib")
install(TARGETS VideoSubFinderCli RUNTIME DESTINATION .)
install(FILES Settings/general.cfg DESTINATION settings)
install(FILES LICENSE THIRD_PARTY.md DESTINATION licenses)
install(FILES 3rdparty/json/LICENSE.MIT DESTINATION licenses RENAME json-LICENSE.MIT)
install(FILES 3rdparty/thread-pool/LICENSE.txt DESTINATION licenses RENAME thread-pool-LICENSE.txt)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
    add_test(NAME cli-regression COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tests/test_cli.py $<TARGET_FILE:VideoSubFinderCli>)
    set_tests_properties(cli-regression PROPERTIES TIMEOUT 180)
endif()
