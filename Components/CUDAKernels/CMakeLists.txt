
project(CUDAKernels LANGUAGES CUDA)

file(GLOB_RECURSE CUDAKernels_src
     "*.h"
     "*.cu"
)

add_library(CUDAKernels STATIC ${CUDAKernels_src})

target_compile_features(CUDAKernels PUBLIC cxx_std_11)

if (WIN32)
	set_target_properties(CUDAKernels PROPERTIES CUDA_ARCHITECTURES all)
else()
	set_target_properties(CUDAKernels PROPERTIES CUDA_ARCHITECTURES "50;52;53;60;61;62;70;72;75;80;86;87;89;90")
endif (WIN32)

target_compile_options(CUDAKernels PRIVATE ${CMAKE_CUDA_FLAGS})
