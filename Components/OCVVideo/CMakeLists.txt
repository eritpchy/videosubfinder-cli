file(GLOB OCVVideo_src
     "*.h"
     "*.cpp"
)

add_library(OCVVideo STATIC ${OCVVideo_src})

target_compile_features(OCVVideo PUBLIC cxx_std_17)

target_include_directories(OCVVideo PUBLIC
	"../Include"
	"$ENV{OPENCV_PATH_X64}/builds/Win64/install/include"
	"$ENV{WX_WIDGETS_PATH}/lib/vc_x64_lib/mswu"
	"$ENV{WX_WIDGETS_PATH}/include"
)
