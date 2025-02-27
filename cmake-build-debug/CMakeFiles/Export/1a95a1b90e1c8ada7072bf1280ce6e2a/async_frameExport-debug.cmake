#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "async_frame::async_frame" for configuration "Debug"
set_property(TARGET async_frame::async_frame APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(async_frame::async_frame PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libasync_frame.so"
  IMPORTED_SONAME_DEBUG "libasync_frame.so"
  )

list(APPEND _cmake_import_check_targets async_frame::async_frame )
list(APPEND _cmake_import_check_files_for_async_frame::async_frame "${_IMPORT_PREFIX}/lib/libasync_frame.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
