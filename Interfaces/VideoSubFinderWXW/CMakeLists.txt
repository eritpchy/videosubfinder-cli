file(GLOB VideoSubFinderWXW_src
     "*.h"
     "*.cpp"
)

add_executable(VideoSubFinderWXW ${VideoSubFinderWXW_src})

target_compile_features(VideoSubFinderWXW PUBLIC cxx_std_17)

target_include_directories(VideoSubFinderWXW PUBLIC
	"$ENV{OPENCV_PATH_X64}/builds/Win64/install/include"
	"$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib/mswu"
	"$ENV{WX_WIDGETS_PATH}/include"
	"../../Components/Include"
	"../../Components/IPAlgorithms"
	"../../Components/OCVVideo"
	"../../Components/FFMPEGVideo"
)

target_link_directories(VideoSubFinderWXW  PUBLIC
	"$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib"
	"$ENV{OPENCV_PATH_X64}/builds/Win64/lib/Release"
	# "$ENV{CUDA_TOOLKIT_PATH}/lib/x64"
	"$ENV{FFMPEG_PATH_X64}/lib"
	# "../../Components/CUDAKernels/x64/Release"
)

target_link_libraries(VideoSubFinderWXW PUBLIC
	IPAlgorithms
	OCVVideo
	FFMPEGVideo
	# CUDAKernels.lib
	wxmsw32u_aui.lib
	wxmsw32u_media.lib
	wxmsw32u_core.lib
	wxmsw32u_adv.lib
	wxbase32u.lib
	wxtiff.lib
	wxjpeg.lib
	wxpng.lib
	wxzlib.lib
	wxregexu.lib
	wxexpat.lib
	winmm.lib
	comctl32.lib
	rpcrt4.lib
	wsock32.lib
	odbc32.lib
	# cudart.lib
	# nppicc.lib
	# nppig.lib
	vfw32.lib
	avdevice.lib
	avformat.lib
	avfilter.lib
	avcodec.lib
	swresample.lib
	swscale.lib
	avutil.lib
	opencv_world470.lib
)
