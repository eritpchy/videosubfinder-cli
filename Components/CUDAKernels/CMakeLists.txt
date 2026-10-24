
project(CUDAKernels LANGUAGES CUDA)

file(GLOB_RECURSE CUDAKernels_src
     "*.h"
     "*.cu"
)

add_library(CUDAKernels STATIC ${CUDAKernels_src})

target_compile_features(CUDAKernels PUBLIC cxx_std_11)

target_compile_options(CUDAKernels PRIVATE ${CMAKE_CUDA_FLAGS})
