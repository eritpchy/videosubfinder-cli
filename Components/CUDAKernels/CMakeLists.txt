file(GLOB_RECURSE CUDAKernels_src
     "*.h"
     "*.cu"
)

add_library(CUDAKernels STATIC ${CUDAKernels_src})

target_compile_features(CUDAKernels PUBLIC cxx_std_11)

set_target_properties(CUDAKernels PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
