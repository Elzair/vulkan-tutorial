#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#include "base-includes.hpp"

uint32_t findMemoryType( VkPhysicalDevice      physical,
                         uint32_t              typeFilter,
                         VkMemoryPropertyFlags props )
{
  VkPhysicalDeviceMemoryProperties memprops;
  vkGetPhysicalDeviceMemoryProperties( physical, &memprops );

  for ( uint32_t i = 0; i < memprops.memoryTypeCount; i++ )
  {
    if ( ( typeFilter & ( 1 << i ) ) &&
         ( memprops.memoryTypes[i].propertyFlags & props ) == props )
    {
      return i;
    }
  }

  throw std::runtime_error( "Failed to find suitable memory type!" );
}

#endif
