file(GLOB FFMPEGVideo_src
     "*.h"
     "*.cpp"
)

add_library(FFMPEGVideo STATIC ${FFMPEGVideo_src})

target_compile_features(FFMPEGVideo PUBLIC cxx_std_17)

target_include_directories(FFMPEGVideo PUBLIC
	"../Include"
	"../../Components/IPAlgorithms"
	${wxWidgets_INCLUDE_DIRS}
	 ${OpenCV_INCLUDE_DIRS}
	 ${FFMPEG_INCLUDE_DIRS}
	 ${CUDAKernels_INCLUDE_DIRS}
)

if (WIN32)
else()
target_compile_definitions(FFMPEGVideo PUBLIC
	${wxWidgets_DEFINITIONS}
	$<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
target_compile_options(FFMPEGVideo PRIVATE ${wxWidgets_CXX_FLAGS})
endif (WIN32)