
#find_package(Vulkan)
#find_package(glfw)
#find_package(glm)

#add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
include_directories(${VULKAN_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
add_executable(lesson08 main.cpp)
target_link_libraries(lesson08 ${VULKAN_LIBRARY} ${GLFW_LIBRARIES})
set_property(TARGET lesson08 PROPERTY CXX_STANDARD 11)
set_property(TARGET lesson08 PROPERTY CXX_STANDARD_REQUIRED ON)


