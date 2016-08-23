#ifndef __VKEXTENSIONS_HPP__
#define __VKEXTENSIONS_HPP__

#include <cstring>
#include "base-includes.hpp"

const std::vector<const char*> required_device_extensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool checkDeviceExtensionSupport( VkPhysicalDevice device )
{
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extension_count, nullptr );
  std::vector<VkExtensionProperties> available_extensions( extension_count );
  vkEnumerateDeviceExtensionProperties( device, nullptr, &extension_count, available_extensions.data() );

  // Ensure all required extensions are available on device
  for ( const auto& required_ext : required_device_extensions )
  {
    bool found = false;

    for ( const auto& ext : available_extensions )
    {
      if ( std::strcmp( ext.extensionName, required_ext ) == 0 )
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
