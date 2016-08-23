#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include "common-includes.hpp"
#include "vkextensions.hpp"
#include "queue.hpp"

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
  VkPhysicalDevice                   physical = VK_NULL_HANDLE;
  VDeleter<VkDevice>                 device   { vkDestroyDevice };
  VkQueue                            q_gfx;
  
  void initWindow()
  {
    glfwInit();
    
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    
    this->window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
  }
    
  void initVulkan()
  {
    this->createInstance();
    this->createDebugCallback();
    this->pickPhysicalDevice();
    this->createLogicalDevice();
  }

  void mainLoop()
  {
    while ( !glfwWindowShouldClose( this->window ) )
    {
      glfwPollEvents();
    }
  }

  void createInstance(  )
  {
    // Ensure validation layers are available
    if ( enable_validation_layers && !this->checkValidationLayerSupport() )
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
    if ( enable_validation_layers )
    {
      crinfo.enabledLayerCount   = validation_layers.size(  );
      crinfo.ppEnabledLayerNames = validation_layers.data(  );
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
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, nullptr );
    std::vector<VkExtensionProperties> extensions( extension_count );
    vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, extensions.data(  ) );

    std::cout << "Available Extensions:" << std::endl;
    for ( const auto& extension: extensions )
    {
      std::cout << "\t" << extension.extensionName << std::endl;
    }
  }

  bool checkValidationLayerSupport(  )
  {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties( &layer_count, nullptr );
    std::vector<VkLayerProperties> available_layers( layer_count );
    vkEnumerateInstanceLayerProperties( &layer_count, available_layers.data(  ) );

    for ( const char* layer : validation_layers )
    {
      bool found = false;

      for ( const auto& properties : available_layers )
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

  void pickPhysicalDevice(  )
  {
    uint32_t dev_count = 0;
    vkEnumeratePhysicalDevices( this->instance, &dev_count, nullptr);
    if ( dev_count == 0 )
    {
      throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
    }
    std::vector<VkPhysicalDevice> devices( dev_count );
    vkEnumeratePhysicalDevices( this->instance, &dev_count, devices.data(  ) );

    for ( const auto& device : devices )
    {
      if ( this->isDeviceSuitable( device ) )
      {
        this->physical = device;
        break;
      }
    }

    if ( this->physical == VK_NULL_HANDLE )
    {
      throw std::runtime_error( "Failed to find a suitable GPU!" );
    }
  }

  void createLogicalDevice(  )
  {
    QueueFamilyIndices indices = findQueueFamilies( this->physical );

    VkDeviceQueueCreateInfo qcrinfo = {};
    qcrinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qcrinfo.queueFamilyIndex = indices.graphics_family;
    qcrinfo.queueCount = 1;
    float qpriority = 1.0f;
    qcrinfo.pQueuePriorities = &qpriority;

    VkPhysicalDeviceFeatures dev_features = {};

    VkDeviceCreateInfo devcrinfo = {};
    devcrinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devcrinfo.pQueueCreateInfos = &qcrinfo;
    devcrinfo.queueCreateInfoCount = 1;
    devcrinfo.pEnabledFeatures = &dev_features;
    devcrinfo.enabledExtensionCount = 0;
    if ( enable_validation_layers )
    {
      devcrinfo.enabledLayerCount = validation_layers.size(  );
      devcrinfo.ppEnabledLayerNames = validation_layers.data(  );
    }
    else
    {
      devcrinfo.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( this->physical, &devcrinfo, nullptr, &this->device ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create logical device!" );
    }

    vkGetDeviceQueue( this->device, indices.graphics_family, 0, &this->q_gfx);
  }

  std::vector<const char*> getRequiredExtensions(  )
  {
    std::vector<const char*> extensions;

    unsigned int glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions( &glfw_extension_count );

    for ( auto i = 0; i < glfw_extension_count; i++ )
    {
      extensions.push_back( glfw_extensions[ i ] );
    }

    if ( enable_validation_layers )
    {
      extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
    }

    return extensions;
  }

  bool isDeviceSuitable( VkPhysicalDevice device )
  {
    auto indices = findQueueFamilies( device );
    return indices.isComplete();
  }

#ifndef WIN32
#define __stdcall
#endif
  static VkBool32 __stdcall debugCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t                   obj,
    std::size_t                loc,
    int32_t                    code,
    const char*                layer_prefix,
    const char*                msg,
    void*                      user_data)
  {
    std::cerr << "Validation layer: " << msg << std::endl;
    return VK_FALSE;
  }

  void createDebugCallback()
  {
    if ( !enable_validation_layers )
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
