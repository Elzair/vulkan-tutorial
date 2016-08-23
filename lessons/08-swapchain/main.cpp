#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <functional>
#include "common.hpp"
#include "queue.hpp"
#include "swapchain.hpp"

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
  VDeleter<VkSurfaceKHR>             surface  { this->instance, vkDestroySurfaceKHR };
  VkPhysicalDevice                   physical = VK_NULL_HANDLE;
  VDeleter<VkDevice>                 device   { vkDestroyDevice };
  VkQueue                            graphics_queue;
  VkQueue                            present_queue;
  VDeleter<VkSwapchainKHR>           swapchain { this->device, vkDestroySwapchainKHR };
  std::vector<VkImage>               swapchain_images;
  VkFormat                           swapchain_image_format;
  VkExtent2D                         swapchain_extent;
  
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
    this->createSurface();
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapChain();
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
    
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_create_info = {};
    inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pApplicationInfo = &app_info;
    auto vkextensions = this->getRequiredExtensions();
    inst_create_info.enabledExtensionCount = vkextensions.size();
    inst_create_info.ppEnabledExtensionNames = vkextensions.data();
    if ( enable_validation_layers )
    {
      inst_create_info.enabledLayerCount   = validation_layers.size(  );
      inst_create_info.ppEnabledLayerNames = validation_layers.data(  );
    }
    else
    {
      inst_create_info.enabledLayerCount = 0;
    }

    if ( vkCreateInstance( &inst_create_info, nullptr, &this->instance ) != VK_SUCCESS )
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

  void createSurface(  )
  {
    if ( glfwCreateWindowSurface( this->instance, this->window, nullptr, &this->surface ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create window surface!" );
    }
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
    // Create queues for both the graphics and presentation families
    QueueFamilyIndices indices = findQueueFamilies( this->physical, this->surface );
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<int> unique_queue_families = {
      indices.graphics_family,
      indices.present_family
    };
    float queue_priority = 1.0f;

    for ( int queue_family : unique_queue_families )
    {
      VkDeviceQueueCreateInfo queue_create_info = {};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queue_priority;
      queue_create_infos.push_back( queue_create_info );
    }

    VkPhysicalDeviceFeatures dev_features = {};

    // Create struct used to create a logical device
    VkDeviceCreateInfo dev_create_info = {};
    dev_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_create_info.pQueueCreateInfos = queue_create_infos.data();
    dev_create_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
    dev_create_info.pEnabledFeatures = &dev_features;
    dev_create_info.enabledExtensionCount = required_device_extensions.size();
    dev_create_info.ppEnabledExtensionNames = required_device_extensions.data();
    if ( enable_validation_layers )
    {
      dev_create_info.enabledLayerCount   = validation_layers.size(  );
      dev_create_info.ppEnabledLayerNames = validation_layers.data(  );
    }
    else
    {
      dev_create_info.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( this->physical, &dev_create_info, nullptr, &this->device ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create logical device!" );
    }

    // Retrieve handles for graphics and presentation queues
    vkGetDeviceQueue( this->device, indices.graphics_family, 0, &this->graphics_queue);
    vkGetDeviceQueue( this->device, indices.present_family,  0, &this->present_queue );
  }

  void createSwapChain( )
  {
    auto swapchain_support = querySwapChainSupport( this->physical, this->surface );
    auto surface_format    = chooseSwapChainSurfaceFormat( swapchain_support.formats );
    auto present_mode      = chooseSwapChainPresentMode( swapchain_support.present_modes );
    auto extent            = chooseSwapChainExtent( swapchain_support.capabilities,
                                                    WIDTH,
                                                    HEIGHT );

    this->swapchain_image_format = surface_format.format;
    this->swapchain_extent       = extent;

    // Ensure we have enough images to properly implement triple buffering
    // A value of 0 for maxImageCount means there is no hard limit on # images
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if ( swapchain_support.capabilities.maxImageCount > 0 &&
         image_count > swapchain_support.capabilities.maxImageCount )
    {
      image_count = swapchain_support.capabilities.maxImageCount;
    }

    // Get indices of graphics and present queues
    auto indices = findQueueFamilies( this->physical, this->surface );
    uint32_t family_indices[] = {
      (uint32_t)indices.graphics_family,
      (uint32_t)indices.present_family
    };

    // Create struct used to create a swapchain
    VkSwapchainCreateInfoKHR swch_create_info = {};
    swch_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swch_create_info.surface          = this->surface;
    swch_create_info.minImageCount    = image_count;
    swch_create_info.imageFormat      = this->swapchain_image_format;
    swch_create_info.imageColorSpace  = surface_format.colorSpace;
    swch_create_info.imageExtent      = this->swapchain_extent;
    swch_create_info.imageArrayLayers = 1;
    swch_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ( indices.graphics_family != indices.present_family )
    {
      swch_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swch_create_info.queueFamilyIndexCount = 2;
      swch_create_info.pQueueFamilyIndices   = family_indices;
    }
    else
    {
      swch_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      swch_create_info.queueFamilyIndexCount = 0;
      swch_create_info.pQueueFamilyIndices   = nullptr;
    }
    swch_create_info.preTransform   = swapchain_support.capabilities.currentTransform;
    swch_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swch_create_info.presentMode    = present_mode;
    swch_create_info.clipped        = VK_TRUE;
    swch_create_info.oldSwapchain   = VK_NULL_HANDLE;

    // Create swapchain
    if ( vkCreateSwapchainKHR( this->device, &swch_create_info, nullptr, &this->swapchain ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create swapchain!" );
    }

    // Retrieve handles to all swapchain images
    // Since the implementation can create more images,
    // we must query the number of images
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &image_count, nullptr );
    this->swapchain_images.resize( image_count );
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &image_count, this->swapchain_images.data() );
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
    auto indices = findQueueFamilies( device, this->surface );

    bool required_extensions_supported = checkDeviceExtensionSupport( device );

    bool adequate_swap_chain = false;
    if ( required_extensions_supported )
    {
      auto swapchain_support = querySwapChainSupport( device, this->surface );
      adequate_swap_chain = !swapchain_support.formats.empty() &&
                            !swapchain_support.present_modes.empty();
    }
    
    return indices.isComplete() && required_extensions_supported && adequate_swap_chain;
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
