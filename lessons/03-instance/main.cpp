#include <iostream>
#include <vector>
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
  GLFWwindow*          window;
  VDeleter<VkInstance> instance { vkDestroyInstance };
  
  void initWindow()
  {
    glfwInit();
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    
    this->window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
  }

  void createInstance(  )
  {
    VkApplicationInfo appinfo = {};
    appinfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pApplicationName   = "Hello Triangle";
    appinfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appinfo.pEngineName        = "No Engine";
    appinfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appinfo.apiVersion         = VK_API_VERSION_1_0;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    VkInstanceCreateInfo crinfo = {};
    crinfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    crinfo.pApplicationInfo        = &appinfo;
    crinfo.enabledExtensionCount   = glfwExtensionCount;
    crinfo.ppEnabledExtensionNames = glfwExtensions;
    crinfo.enabledLayerCount       = 0;

    if ( vkCreateInstance( &crinfo, nullptr, &this->instance ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create instance!" );
    }
    else
    {
      std::cout << "Created instance!" << std::endl;
    }

    // Get information on extension properties
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
    std::vector<VkExtensionProperties> extensions( extensionCount );
    vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data(  ) );

    std::cout << "Available Extensions:" << std::endl;
    for ( const auto& extension: extensions )
    {
      std::cout << "\t" << extension.extensionName << std::endl;
    }
  }
    
  void initVulkan()
  {
    this->createInstance(  );
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
