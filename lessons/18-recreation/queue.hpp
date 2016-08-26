#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include "base-includes.hpp"

struct QueueFamilyIndices
{
  int graphicsFamily = -1;
  int presentFamily  = -1;

  bool isComplete(  )
  {
    return this->graphicsFamily >= 0 && this->presentFamily >= 0;
  }
};

QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
  QueueFamilyIndices indices;

  uint32_t qfCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( device, &qfCount, nullptr );
  std::vector<VkQueueFamilyProperties> qfamilies( qfCount );
  vkGetPhysicalDeviceQueueFamilyProperties( device, &qfCount, qfamilies.data() );

  int i = 0;
  for ( const auto& qfamily : qfamilies )
  {
    if ( qfamily.queueCount > 0 && qfamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );
    if ( qfamily.queueCount > 0 && presentSupport )
    {
      indices.presentFamily = i;
    }

    if ( indices.isComplete() )
    {
      break;
    }

    i++;
  }

  return indices;
}

#endif
