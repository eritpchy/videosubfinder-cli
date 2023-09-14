SET(VideoSubFinderCli_src
    VideoSubFinderCli.h
    VideoSubFinderCli.cpp
   )

add_executable(VideoSubFinderCli ${VideoSubFinderCli_src})


target_compile_features(VideoSubFinderCli PUBLIC cxx_std_17)

target_include_directories(VideoSubFinderCli PUBLIC
	${wxWidgets_INCLUDE_DIRS}
	 ${OpenCV_INCLUDE_DIRS}
	"../../Components/Include"
	"../../Components/IPAlgorithms"
	"../../Components/OCVVideo"
	"../../Components/FFMPEGVideo"
)

if (WIN32)
	target_link_directories(VideoSubFinderCli  PUBLIC
		${VideoSubFinderCli_LINK_DIRS}
	)

	target_link_libraries(VideoSubFinderCli PUBLIC
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
	target_compile_definitions(VideoSubFinderCli PUBLIC
		${wxWidgets_DEFINITIONS}
		$<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
	target_compile_options(VideoSubFinderCli PRIVATE ${wxWidgets_CXX_FLAGS})

	target_link_directories(VideoSubFinderCli  PUBLIC
		"${wxWidgets_LIBRARY_DIRS}"
	)
			
	target_link_libraries(VideoSubFinderCli PUBLIC
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
    )

    if (BUILD_FULL_STATIC)
    	message("BUILD_FULL_STATIC is ON")
		target_link_libraries(VideoSubFinderCli PUBLIC
			"-static"
			"SDL2"
			"SDL2_test"
			"SDL2main"
			"SvtAv1Dec"
			"SvtAv1Enc"
			"aom"
			"avdevice"
			"crypto"
			"fdk-aac"
			"gif"
			"ltdl"
			"mp3lame"
			"ogg"
			"opencore-amrnb"
			"opencore-amrwb"
			"opus"
			"png"
			"png16"
			"postproc"
			"srt"
			"ssl"
			"swresample"
			"theora"
			"theoradec"
			"theoraenc"
			"tiff"
			"tiffxx"
			"vidstab"
			"vorbis"
			"vorbisenc"
			"vorbisfile"
			"vpx"
			"webp"
			"webpdecoder"
			"webpdemux"
			"webpmux"
			"x264"
			"x265"
			"xvidcore"
			"yasm"
			"z"
			"zimg"	
		)
	endif(BUILD_FULL_STATIC)


endif (WIN32)

if (APPLE)
	target_link_libraries(VideoSubFinderCli PUBLIC
        wx_osx_cocoau_core-3.2
    )
endif(APPLE)
