file(GLOB VideoSubFinderWXW_src
     "*.h"
     "*.cpp"
)

if (WIN32)
	add_executable(VideoSubFinderWXW WIN32 ${VideoSubFinderWXW_src} VideoSubFinderWXW.rc)
else()
	add_executable(VideoSubFinderWXW ${VideoSubFinderWXW_src})
endif (WIN32)

target_compile_features(VideoSubFinderWXW PUBLIC cxx_std_17)

target_include_directories(VideoSubFinderWXW PUBLIC
	${wxWidgets_INCLUDE_DIRS}
	 ${OpenCV_INCLUDE_DIRS}
	"../../Components/Include"
	"../../Components/IPAlgorithms"
	"../../Components/OCVVideo"
	"../../Components/FFMPEGVideo"
)

if (WIN32)
	target_link_directories(VideoSubFinderWXW  PUBLIC
		${VideoSubFinderWXW_LINK_DIRS}		
	)

	target_link_libraries(VideoSubFinderWXW PUBLIC
		IPAlgorithms
		OCVVideo
		FFMPEGVideo
		${CUDAKernels_LIB}
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
		vfw32.lib
		avdevice.lib
		avformat.lib
		avfilter.lib
		avcodec.lib
		swresample.lib
		swscale.lib
		avutil.lib		
		${OpenCV_LIBS}
	)	
else()
	target_compile_definitions(VideoSubFinderWXW PUBLIC
		${wxWidgets_DEFINITIONS}
		$<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
	target_compile_options(VideoSubFinderWXW PRIVATE ${wxWidgets_CXX_FLAGS})

	target_link_directories(VideoSubFinderWXW  PUBLIC
		${wxWidgets_LIBRARY_DIRS}
		${CUDA_LINK_DIRS}
	)

	target_link_libraries(VideoSubFinderWXW PUBLIC
		IPAlgorithms
		OCVVideo
		FFMPEGVideo
		${CUDAKernels_LIB}
		${wxWidgets_LIBRARIES}
		${OpenCV_LIBS}
		avcodec
		avformat
		avutil
		swscale
		avfilter
		tbb
		X11
	)
endif (WIN32)


