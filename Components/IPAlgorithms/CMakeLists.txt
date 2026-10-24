file(GLOB IPAlgorithms_src
     "*.h"
     "*.cpp"
)

add_library(IPAlgorithms STATIC ${IPAlgorithms_src})

target_compile_features(IPAlgorithms PUBLIC cxx_std_17)

target_include_directories(IPAlgorithms PUBLIC
	"../Include"
	 ${OpenCV_INCLUDE_DIRS}
	 ${CUDAKernels_INCLUDE_DIRS}
)


