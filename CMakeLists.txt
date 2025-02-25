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
find_package(OpenVINO REQUIRED COMPONENTS Runtime ONNX)
#cuda
find_package(CUDA REQUIRED)
if (CUDA_FOUND)
    message(STATUS "CUDA is Found")
    find_package(TensorRT REQUIRED)
    set(CMAKE_CUDA_ARCHITECTURES 60 61 62 70 72 75 86 89 90)
    set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
    print_var(CUDA_LIBRARIES)
    print_var(CUDA_INCLUDE_DIRS)
    print_var(TensorRT_INCLUDE_DIRS)
    print_var(TensorRT_LIBRARIES)
    print_var(CUDA_LIB_DIR)
    include_directories(${CUDA_INCLUDE_DIRS} ${TensorRT_INCLUDE_DIRS})
    add_definitions(-DTRT)
    ament_export_definitions(TRT)
    #get_filename_component(CUDA_LIB_DIR ${CUDA_LIBRARIES} DIRECTORY)
endif ()

# include
include_directories(include)

# include
include_directories(include)

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
        ${OpenCV_INCLUDE_DIRS} ${CERES_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
        ${OpenCV_LIBS}
        openvino::frontend::onnx openvino::runtime)

if(CUDA_FOUND)
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
if(BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    list(APPEND AMENT_LINT_AUTO_EXCLUDE
            ament_cmake_copyright
            ament_cmake_uncrustify
            ament_cmake_cpplint
    )
    ament_lint_auto_find_test_dependencies()
endif()

#############################################33
#add_executable(testH test.cpp src/threadPool/threadPool.cpp src/inferer/AsyncInferer.cpp)
#target_link_libraries(testH ${OpenCV_LIBS})
#target_link_libraries(testH ${PROJECT_NAME})
################################################3
ament_package()