
#find_package(Vulkan)
#find_package(glfw)
#find_package(glm)

#add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
include_directories(${VULKAN_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
add_executable(lesson25 main.cpp)
target_link_libraries(lesson25 ${VULKAN_LIBRARY} ${GLFW_LIBRARIES})
set_property(TARGET lesson25 PROPERTY CXX_STANDARD 11)
set_property(TARGET lesson25 PROPERTY CXX_STANDARD_REQUIRED ON)

compile_shader(lesson25 shader.vert vert.spv)
compile_shader(lesson25 shader.frag frag.spv)

file(COPY texture.jpg DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
