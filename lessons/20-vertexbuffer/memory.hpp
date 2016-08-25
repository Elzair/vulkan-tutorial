#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include "base-includes.hpp"

uint32_t findMemoryType( VkPhysicalDevice phys_dev, uint32_t type_filter, VkMemoryPropertyFlags props )
{
  VkPhysicalDeviceMemoryProperties memprops;
  vkGetPhysicalDeviceMemoryProperties( phys_dev, &memprops );

  for ( uint32_t i = 0; i < memprops.memoryTypeCount; i++ )
  {
    if ( ( type_filter & ( 1 << i ) ) &&
         ( memprops.memoryTypes[i].propertyFlags & props ) == props )
    {
      return i;
    }
  }

  throw std::runtime_error( "Failed to find suitable memory type!" );
}

#endif
