file(GLOB IPAlgorithms_src
     "*.h"
     "*.cpp"
)

add_library(IPAlgorithms STATIC ${IPAlgorithms_src})

target_compile_features(IPAlgorithms PUBLIC cxx_std_17)

target_include_directories(IPAlgorithms PUBLIC
	"../Include"
	 ${wxWidgets_INCLUDE_DIRS}
	 ${OpenCV_INCLUDE_DIRS}
	 ${CUDAKernels_INCLUDE_DIRS}
)

if (WIN32)
else()
target_compile_definitions(IPAlgorithms PUBLIC
	${wxWidgets_DEFINITIONS}
	$<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
target_compile_options(IPAlgorithms PRIVATE ${wxWidgets_CXX_FLAGS})
endif (WIN32)


