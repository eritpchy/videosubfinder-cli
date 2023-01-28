cmake_minimum_required(VERSION 3.13)

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

add_subdirectory(Components/IPAlgorithms)
add_subdirectory(Components/OCVVideo)
add_subdirectory(Components/FFMPEGVideo)
add_subdirectory(Interfaces/VideoSubFinderWXW)
