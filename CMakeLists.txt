cmake_minimum_required(VERSION 3.5)
project(AsyncInferFrame)

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
    #get_filename_component(CUDA_LIB_DIR ${CUDA_LIBRARIES} DIRECTORY)
endif ()
# include
include_directories(include)
file(GLOB_RECURSE LIB_SRC src/inferer/*.cpp src/threadPool/*.cpp)
add_library(${PROJECT_NAME} SHARED ${LIB_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS} ${CERES_INCLUDE_DIRS})

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

##########################################################################
add_executable(test src/test.cpp)
target_link_libraries(test
        ${PROJECT_NAME})

add_executable(testTP testThreadPool.cpp)
target_link_libraries(testTP
        ${PROJECT_NAME})
##########################################################################
