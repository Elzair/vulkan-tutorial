
#find_package(Vulkan)
#find_package(glfw)
#find_package(glm)

#add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
include_directories(${VULKAN_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
add_executable(lesson18 main.cpp)
target_link_libraries(lesson18 ${VULKAN_LIBRARY} ${GLFW_LIBRARIES})
set_property(TARGET lesson18 PROPERTY CXX_STANDARD 11)
set_property(TARGET lesson18 PROPERTY CXX_STANDARD_REQUIRED ON)

compile_shader(lesson18 shader.vert vert.spv)
compile_shader(lesson18 shader.frag frag.spv)
