cmake_minimum_required(VERSION 3.5)
project(cross_platform)
# find package
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

include(CMakePackageConfigHelpers)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i686|i386")
    message(STATUS "Detected x86 architecture")
    set(DEVICE_ARCH "x86")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    message(STATUS "Detected AArch64 architecture")
    set(DEVICE_ARCH "AArch64")
else ()
    message(FATAL_ERROR "Processor architecture not recognized: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

find_package(CUDA)
if (CUDA_FOUND)
    message(STATUS "Found CUDA")
    set(CUDA_AVAILABLE TRUE)
    list(APPEND libs ${CUDA_LIBRARIES})
    set(dependencies ${dependencies} CUDA)
    list(APPEND include_dir ${CUDA_INCLUDE_DIRS})
    find_package(TensorRT)
    if (TensorRT_FOUND)
        message(STATUS "FOUND TensorRT")
        set(TensorRT_AVAILABLE TRUE)
        set(CMAKE_CUDA_ARCHITECTURES 60 61 62 70 72 75 86 89 90)
        set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
        list(APPEND libs ${TensorRT_LIBRARIES})
        list(APPEND include_dir ${TensorRT_INCLUDE_DIRS})
    else ()
        set(TensorRT_AVAILABLE FALSE)
    endif ()
else ()
    set(CUDA_AVAILABLE FALSE)
endif ()


find_package(OpenVINO)
if (OpenVINO_FOUND)
    message(STATUS "Found OpenVINO")
    set(OpenVINO_AVAILABLE TRUE)
    list(APPEND libs openvino::frontend::onnx openvino::runtime)
    set(dependencies ${dependencies} OpenVINO)
else ()
    set(OpenVINO_AVAILABLE FALSE)
endif ()


add_executable(${PROJECT_NAME} main.cpp)

message(STATUS "Export include:${include_dir}")
ament_export_include_directories(${include_dir})
message(STATUS "Export library:${libs}")
ament_export_libraries(${libs})
ament_export_dependencies(${dependencies})

configure_package_config_file(
        cmake/crossConfig.cmake.in
        ${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION share/${PROJECT_NAME}/cmake
)

install(
        FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        DESTINATION share/${PROJECT_NAME}/cmake
)

install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_export
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

ament_export_targets(${PROJECT_NAME}_export HAS_LIBRARY_TARGET)

ament_package()
