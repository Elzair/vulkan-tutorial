#ifndef __SWAPCHAIN_HPP__
#define __SWAPCHAIN_HPP__

#include <limits>

#include "base-includes.hpp"

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   presentModes;
};

SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device,
                                               VkSurfaceKHR     surface )
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );
  if ( formatCount > 0 )
  {
    details.formats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
  }

  uint32_t presentModesCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModesCount, nullptr );
  if ( presentModesCount > 0 )
  {
    details.presentModes.resize( presentModesCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModesCount, details.presentModes.data() );
  }

  return details;
}

VkSurfaceFormatKHR chooseSwapChainSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
{
  // First, check if surface has no preferred format
  if ( availableFormats.size(  ) == 1 && availableFormats[ 0 ].format == VK_FORMAT_UNDEFINED )
  {
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  }

  // Next, check if our preferred format is in the list
  for ( const auto& format : availableFormats )
  {
    if ( format.format == VK_FORMAT_B8G8R8A8_UNORM &&
         format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
    {
      return format;
    }
  }

  // Otherwise, default to first format
  return availableFormats[ 0 ];
}

VkPresentModeKHR chooseSwapChainPresentMode( const std::vector<VkPresentModeKHR> availablePresentModes )
{
  for ( const auto& presentMode : availablePresentModes )
  {
    if ( presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      return presentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR; // Guranteed to be available
}

VkExtent2D chooseSwapChainExtent( const VkSurfaceCapabilitiesKHR& capabilities,
                                  int                             width,
                                  int                             height )
{
  if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
  {
    return capabilities.currentExtent;
  }
  else
  {
    VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
    actualExtent.width = std::max( capabilities.minImageExtent.width,
                                    std::min( capabilities.maxImageExtent.width,
                                              actualExtent.width ) );
    actualExtent.height = std::max( capabilities.minImageExtent.height,
                                    std::min( capabilities.maxImageExtent.height,
                                              actualExtent.height ) );

    return actualExtent;
  }
}

#endif
