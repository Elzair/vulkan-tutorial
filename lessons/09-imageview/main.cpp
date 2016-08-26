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
  VkQueue                            graphicsQueue;
  VkQueue                            presentQueue;
  VDeleter<VkSwapchainKHR>           swapchain { this->device, vkDestroySwapchainKHR };
  std::vector<VkImage>               swapchainImages;
  VkFormat                           swapchainImageFormat;
  VkExtent2D                         swapchainExtent;
  std::vector<VDeleter<VkImageView>> swapchainImageViews;
  
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
    this->createImageViews();
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
    if ( enableValidationLayers && !this->checkValidationLayerSupport() )
    {
      throw std::runtime_error( "Validation layers requested but are not available!" );
    }
    
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instCreateInfo    = {};
    instCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo        = &appInfo;
    auto vkextensions                        = this->getRequiredExtensions();
    instCreateInfo.enabledExtensionCount   = vkextensions.size();
    instCreateInfo.ppEnabledExtensionNames = vkextensions.data();
    if ( enableValidationLayers )
    {
      instCreateInfo.enabledLayerCount   = validationLayers.size(  );
      instCreateInfo.ppEnabledLayerNames = validationLayers.data(  );
    }
    else
    {
      instCreateInfo.enabledLayerCount = 0;
    }

    if ( vkCreateInstance( &instCreateInfo, nullptr, &this->instance ) != VK_SUCCESS )
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

  void createSurface(  )
  {
    if ( glfwCreateWindowSurface( this->instance, this->window, nullptr, &this->surface ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create window surface!" );
    }
  }

  void pickPhysicalDevice(  )
  {
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices( this->instance, &devCount, nullptr);
    if ( devCount == 0 )
    {
      throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
    }
    std::vector<VkPhysicalDevice> devices( devCount );
    vkEnumeratePhysicalDevices( this->instance, &devCount, devices.data(  ) );

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
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {
      indices.graphicsFamily,
      indices.presentFamily
    };
    float queuePriority = 1.0f;

    for ( int queueFamily : uniqueQueueFamilies )
    {
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back( queueCreateInfo );
    }

    VkPhysicalDeviceFeatures devFeatures = {};

    // Create struct used to create a logical device
    VkDeviceCreateInfo devCreateInfo = {};
    devCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    devCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    devCreateInfo.pEnabledFeatures        = &devFeatures;
    devCreateInfo.enabledExtensionCount   = requiredDeviceExtensions.size();
    devCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    if ( enableValidationLayers )
    {
      devCreateInfo.enabledLayerCount   = validationLayers.size(  );
      devCreateInfo.ppEnabledLayerNames = validationLayers.data(  );
    }
    else
    {
      devCreateInfo.enabledLayerCount = 0;
    }

    if ( vkCreateDevice( this->physical, &devCreateInfo, nullptr, &this->device ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create logical device!" );
    }

    // Retrieve handles for graphics and presentation queues
    vkGetDeviceQueue( this->device, indices.graphicsFamily, 0, &this->graphicsQueue);
    vkGetDeviceQueue( this->device, indices.presentFamily,  0, &this->presentQueue );
  }

  void createSwapChain( )
  {
    auto swapchainSupport = querySwapChainSupport( this->physical, this->surface );
    auto surfaceFormat    = chooseSwapChainSurfaceFormat( swapchainSupport.formats );
    auto presentMode      = chooseSwapChainPresentMode( swapchainSupport.presentModes );
    auto extent            = chooseSwapChainExtent( swapchainSupport.capabilities,
                                                    WIDTH,
                                                    HEIGHT );

    this->swapchainImageFormat = surfaceFormat.format;
    this->swapchainExtent       = extent;

    // Ensure we have enough images to properly implement triple buffering
    // A value of 0 for maxImageCount means there is no hard limit on # images
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if ( swapchainSupport.capabilities.maxImageCount > 0 &&
         imageCount > swapchainSupport.capabilities.maxImageCount )
    {
      imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    // Get indices of graphics and present queues
    auto indices = findQueueFamilies( this->physical, this->surface );
    uint32_t familyIndices[] = {
      (uint32_t)indices.graphicsFamily,
      (uint32_t)indices.presentFamily
    };

    // Create struct used to create a swapchain
    VkSwapchainCreateInfoKHR swchCreateInfo = {};
    swchCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swchCreateInfo.surface          = this->surface;
    swchCreateInfo.minImageCount    = imageCount;
    swchCreateInfo.imageFormat      = this->swapchainImageFormat;
    swchCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
    swchCreateInfo.imageExtent      = this->swapchainExtent;
    swchCreateInfo.imageArrayLayers = 1;
    swchCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ( indices.graphicsFamily != indices.presentFamily )
    {
      swchCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swchCreateInfo.queueFamilyIndexCount = 2;
      swchCreateInfo.pQueueFamilyIndices   = familyIndices;
    }
    else
    {
      swchCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      swchCreateInfo.queueFamilyIndexCount = 0;
      swchCreateInfo.pQueueFamilyIndices   = nullptr;
    }
    swchCreateInfo.preTransform   = swapchainSupport.capabilities.currentTransform;
    swchCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swchCreateInfo.presentMode    = presentMode;
    swchCreateInfo.clipped        = VK_TRUE;
    swchCreateInfo.oldSwapchain   = VK_NULL_HANDLE;

    // Create swapchain
    if ( vkCreateSwapchainKHR( this->device, &swchCreateInfo, nullptr, &this->swapchain ) != VK_SUCCESS )
    {
      throw std::runtime_error( "Failed to create swapchain!" );
    }

    // Retrieve handles to all swapchain images
    // Since the implementation can create more images,
    // we must query the number of images
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &imageCount, nullptr );
    this->swapchainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( this->device, this->swapchain,
                             &imageCount, this->swapchainImages.data() );
  }

  void createImageViews(  )
  {
    this->swapchainImageViews.resize( this->swapchainImages.size(  ),
                                        VDeleter<VkImageView>{ this->device,
                                                               vkDestroyImageView } );

    for ( uint32_t i = 0; i < this->swapchainImages.size(  ); i++ )
    {
      VkImageViewCreateInfo imgviewCreateInfo = {};
      imgviewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imgviewCreateInfo.image                           = this->swapchainImages[ i ];
      imgviewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
      imgviewCreateInfo.format                          = this->swapchainImageFormat;
      imgviewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
      imgviewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      imgviewCreateInfo.subresourceRange.baseMipLevel   = 0;
      imgviewCreateInfo.subresourceRange.levelCount     = 1;
      imgviewCreateInfo.subresourceRange.baseArrayLayer = 0;
      imgviewCreateInfo.subresourceRange.layerCount     = 1;

      if ( vkCreateImageView( this->device, &imgviewCreateInfo, nullptr, &this->swapchainImageViews[ i ]) != VK_SUCCESS )
      {
        throw std::runtime_error( "Failed to create image views!" );
      }
    }
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

  bool isDeviceSuitable( VkPhysicalDevice device )
  {
    auto indices = findQueueFamilies( device, this->surface );

    bool requiredExtensionsSupported = checkDeviceExtensionSupport( device );

    bool adequateSwapChain = false;
    if ( requiredExtensionsSupported )
    {
      auto swapchainSupport = querySwapChainSupport( device, this->surface );
      adequateSwapChain = !swapchainSupport.formats.empty() &&
                            !swapchainSupport.presentModes.empty();
    }
    
    return indices.isComplete() && requiredExtensionsSupported && adequateSwapChain;
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
