#include <stdlib.h>
#include <cstring>
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
  GLFWwindow*                        window;
  VDeleter<VkInstance>               instance { vkDestroyInstance };
  VDeleter<VkDebugReportCallbackEXT> callback { this->instance, DestroyDebugReportCallbackEXT };
  
  void initWindow()
  {
    glfwInit();
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    
    this->window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
  }

  bool checkValidationLayerSupport(  )
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );

    for ( const char* layer : validationLayers )
    {
      bool found = false;

      for ( const auto& properties : availableLayers )
      {
        if ( std::strcmp( layer, properties.layerName ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        return false;
      }
    }

    return true;
  }

  std::vector<const char*> getRequiredExtensions(  )
  {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    for ( auto i = 0; i < glfwExtensionCount; i++ )
    {
      extensions.push_back( glfwExtensions[ i ] );
    }

    if ( enableValidationLayers )
    {
      extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
    }

    return extensions;
  }

  void createInstance(  )
  {
    // Ensure validation layers are available
    if ( enableValidationLayers && !this->checkValidationLayerSupport() )
    {
      throw std::runtime_error( "Validation layers requested but are not available!" );
    }
    
    VkApplicationInfo appinfo = {};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pApplicationName = "Hello Triangle";
    appinfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appinfo.pEngineName = "No Engine";
    appinfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appinfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo crinfo = {};
    crinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    crinfo.pApplicationInfo = &appinfo;
    auto vkextensions = this->getRequiredExtensions();
    crinfo.enabledExtensionCount = vkextensions.size();
    crinfo.ppEnabledExtensionNames = vkextensions.data();
    if ( enableValidationLayers )
    {
      crinfo.enabledLayerCount   = validationLayers.size(  );
      crinfo.ppEnabledLayerNames = validationLayers.data(  );
    }
    else
    {
      crinfo.enabledLayerCount = 0;
    }

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
    this->createInstance();
    this->createDebugCallback();
  }

  void mainLoop()
  {
    while ( !glfwWindowShouldClose( this->window ) )
    {
      glfwPollEvents();
    }
  }

#ifndef WIN32
#define __stdcall
#endif
  static VkBool32 __stdcall debugCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t                   obj,
    std::size_t                loc,
    int32_t                    code,
    const char*                layerPrefix,
    const char*                msg,
    void*                      userData)
  {
    std::cerr << "Validation layer: " << msg << std::endl;
    return VK_FALSE;
  }

  void createDebugCallback()
  {
    if ( !enableValidationLayers )
    {
      return;
    }
    
    VkDebugReportCallbackCreateInfoEXT crinfo = {};
    crinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    crinfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    crinfo.pfnCallback = this->debugCallback;

    if ( CreateDebugReportCallbackEXT( this->instance,
                                       &crinfo,
                                       nullptr,
                                       &this->callback ) )
    {
      throw std::runtime_error( "Failed to setu up debug callback!" );
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
