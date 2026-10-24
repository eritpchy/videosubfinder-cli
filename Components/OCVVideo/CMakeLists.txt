file(GLOB OCVVideo_src
     "*.h"
     "*.cpp"
)

add_library(OCVVideo STATIC ${OCVVideo_src})

target_compile_features(OCVVideo PUBLIC cxx_std_17)

target_include_directories(OCVVideo PUBLIC
	"../Include"
	 ${OpenCV_INCLUDE_DIRS}
)

