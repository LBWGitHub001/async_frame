# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_async_frame_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED async_frame_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(async_frame_FOUND FALSE)
  elseif(NOT async_frame_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(async_frame_FOUND FALSE)
  endif()
  return()
endif()
set(_async_frame_CONFIG_INCLUDED TRUE)

# output package information
if(NOT async_frame_FIND_QUIETLY)
  message(STATUS "Found async_frame: 0.9.0 (${async_frame_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'async_frame' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT ${async_frame_DEPRECATED_QUIET})
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(async_frame_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "ament_cmake_export_targets-extras.cmake")
foreach(_extra ${_extras})
  include("${async_frame_DIR}/${_extra}")
endforeach()
