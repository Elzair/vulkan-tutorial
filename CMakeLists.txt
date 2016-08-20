if(APPLE)
  cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
else()
  cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)
endif()
 
project(Tutorial)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
include(AppendCmakeInstallRPath)
include(FindVulkan)

#find_package(Git REQUIRED)
find_package(Vulkan)

find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)

# =============================================================================
#
# Build lessons
#
# =============================================================================

add_subdirectory(lessons)
