file(GLOB OCVVideo_src
     "*.h"
     "*.cpp"
)

add_library(OCVVideo STATIC ${OCVVideo_src})

target_compile_features(OCVVideo PUBLIC cxx_std_17)

target_include_directories(OCVVideo PUBLIC
	"../Include"
	 ${wxWidgets_INCLUDE_DIRS}
	 ${OpenCV_INCLUDE_DIRS}
)

if (WIN32)
else()

target_compile_definitions(OCVVideo PUBLIC
	${wxWidgets_DEFINITIONS}
	$<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
target_compile_options(OCVVideo PRIVATE ${wxWidgets_CXX_FLAGS})
endif (WIN32)