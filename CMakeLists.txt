cmake_minimum_required(VERSION 3.5)
project(async_frame)

# Default to C++17
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Function)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find package
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(OpenCV 4 REQUIRED)
find_package(cross_platform REQUIRED)

include(${cross_platform_DIR}/cross_platformConfig.cmake)
message(STATUS CUDA:${CUDA_AVAILABLE})
message(STATUS TensonrRT:${TensorRT_AVAILABLE})
message(STATUS OpenVINO:${OpenVINO_AVAILABLE})

if (${OpenVINO_AVAILABLE})
    find_package(OpenVINO REQUIRED COMPONENTS Runtime ONNX)
    add_definitions(-DVINO)
endif ()

if(${TensorRT_AVAILABLE})
    add_definitions(-DTRT)
endif ()

# include
include_directories(include)
if (${CUDA_AVAILABLE})
    include_directories(${CUDA_INCLUDE_DIRS} ${TensorRT_INCLUDE_DIRS})
endif ()

#Add&Compile source
file(GLOB_RECURSE LIB_SRC src/inferer/*.cpp src/threadPool/*.cpp)
add_library(${PROJECT_NAME} SHARED ${LIB_SRC})

set(dependencies
        rclcpp
        rcpputils
        OpenCV
)

#Link libs
ament_target_dependencies(${PROJECT_NAME} ${dependencies})
target_include_directories(${PROJECT_NAME} PUBLIC
        ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
        ${OpenCV_LIBS})

if (${OpenVINO_AVAILABLE})
    target_link_libraries(${PROJECT_NAME}
    openvino::frontend::onnx openvino::runtime)
endif ()
if (${CUDA_AVAILABLE})
    target_link_libraries(${PROJECT_NAME}
            ${CUDA_LIBRARIES}
            ${TensorRT_LIBRARIES})
endif ()

# Install include directories
install(DIRECTORY include/
        DESTINATION include
)

# Install libraries
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

# export lib lib
ament_export_targets(${PROJECT_NAME} HAS_LIBRARY_TARGET)

# test
if (BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    list(APPEND AMENT_LINT_AUTO_EXCLUDE
            ament_cmake_copyright
            ament_cmake_uncrustify
            ament_cmake_cpplint
    )
    ament_lint_auto_find_test_dependencies()
endif ()

ament_package()