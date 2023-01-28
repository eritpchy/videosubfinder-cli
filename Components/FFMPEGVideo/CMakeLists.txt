file(GLOB FFMPEGVideo_src
     "*.h"
     "*.cpp"
)

add_library(FFMPEGVideo STATIC ${FFMPEGVideo_src})

target_compile_features(FFMPEGVideo PUBLIC cxx_std_17)

target_include_directories(FFMPEGVideo PUBLIC
	"../Include"
	"$ENV{OPENCV_PATH_X64}/builds/Win64/install/include"
	"$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib/mswu"
	"$ENV{WX_WIDGETS_PATH}/include"
	"$ENV{FFMPEG_PATH_X64}/include"
	# "../CUDAKernels"
	"../../Components/IPAlgorithms"
)
