file(GLOB IPAlgorithms_src
     "*.h"
     "*.cpp"
)

add_library(IPAlgorithms STATIC ${IPAlgorithms_src})

target_compile_features(IPAlgorithms PUBLIC cxx_std_17)

target_include_directories(IPAlgorithms PUBLIC
	"../Include"
	# "../CUDAKernels"
	"$ENV{OPENCV_PATH_X64}/builds/Win64/install/include"
	"$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib/mswu"
	"$ENV{WX_WIDGETS_PATH}/include"
)

