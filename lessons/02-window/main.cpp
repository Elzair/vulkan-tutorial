#include <iostream>
#include <stdexcept>
#include <functional>
#include "common.hpp"

class HelloTriangleApplication
{
public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
  }

private:
  GLFWwindow* window;
  
  void initWindow()
  {
    glfwInit();
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    
    this->window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
    
  }
    
  void initVulkan()
  {
        
  }

  void mainLoop()
  {
    while ( !glfwWindowShouldClose( this->window ) )
    {
      glfwPollEvents();
    }
  }
};

int main( )
{
  HelloTriangleApplication app;

  try
  {
    app.run();
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
