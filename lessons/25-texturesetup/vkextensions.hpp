#ifndef __VKEXTENSIONS_HPP__
#define __VKEXTENSIONS_HPP__

#include <cstring>
#include "base-includes.hpp"

const std::vector<const char*> requiredDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool checkDeviceExtensionSupport( VkPhysicalDevice device )
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );
  std::vector<VkExtensionProperties> availableExtensions( extensionCount );
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

  // Ensure all required extensions are available on device
  for ( const auto& requiredExt : requiredDeviceExtensions )
  {
    bool found = false;

    for ( const auto& ext : availableExtensions )
    {
      if ( std::strcmp( ext.extensionName, requiredExt ) == 0 )
      {
        found = true;
      }
    }

    if ( !found )
    {
      return false;
    }
  }

  return true;
}

VkResult CreateDebugReportCallbackEXT(
  VkInstance                                instance,
  const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks*              pAllocator,
  VkDebugReportCallbackEXT*                 pCallback)
{
  auto func = (PFN_vkCreateDebugReportCallbackEXT)
               vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

  if (func != nullptr)
  {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  }
  else
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugReportCallbackEXT(VkInstance                   instance,
                                   VkDebugReportCallbackEXT     callback,
                                   const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugReportCallbackEXT)
              vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  if (func != nullptr)
  {
    func(instance, callback, pAllocator);
  }
}

#endif
