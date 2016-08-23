#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include "base-includes.hpp"

struct QueueFamilyIndices
{
  int graphics_family = -1;

  bool isComplete(  )
  {
    return this->graphics_family >= 0;
  }
};

QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device )
{
  QueueFamilyIndices indices;

  uint32_t qf_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( device, &qf_count, nullptr );
  std::vector<VkQueueFamilyProperties> qfamilies( qf_count );
  vkGetPhysicalDeviceQueueFamilyProperties( device, &qf_count, qfamilies.data() );

  int i = 0;
  for ( const auto& qfamily : qfamilies )
  {
    if ( qfamily.queueCount > 0 && qfamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
    {
      indices.graphics_family = i;
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
