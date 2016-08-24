#ifndef __SWAPCHAIN_HPP__
#define __SWAPCHAIN_HPP__

#include <limits>

#include "base-includes.hpp"

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   present_modes;
};

SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device,
                                               VkSurfaceKHR     surface )
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, nullptr );
  if ( format_count > 0 )
  {
    details.formats.resize( format_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, details.formats.data() );
  }

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &present_modes_count, nullptr );
  if ( present_modes_count > 0 )
  {
    details.present_modes.resize( present_modes_count );
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &present_modes_count, details.present_modes.data() );
  }

  return details;
}

VkSurfaceFormatKHR chooseSwapChainSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& available_formats )
{
  // First, check if surface has no preferred format
  if ( available_formats.size(  ) == 1 && available_formats[ 0 ].format == VK_FORMAT_UNDEFINED )
  {
    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  }

  // Next, check if our preferred format is in the list
  for ( const auto& format : available_formats )
  {
    if ( format.format == VK_FORMAT_B8G8R8A8_UNORM &&
         format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
    {
      return format;
    }
  }

  // Otherwise, default to first format
  return available_formats[ 0 ];
}

VkPresentModeKHR chooseSwapChainPresentMode( const std::vector<VkPresentModeKHR> available_present_modes )
{
  for ( const auto& present_mode : available_present_modes )
  {
    if ( present_mode == VK_PRESENT_MODE_MAILBOX_KHR )
    {
      return present_mode;
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
    VkExtent2D actual_extent = { (uint32_t)width, (uint32_t)height };
    actual_extent.width = std::max( capabilities.minImageExtent.width,
                                    std::min( capabilities.maxImageExtent.width,
                                              actual_extent.width ) );
    actual_extent.height = std::max( capabilities.minImageExtent.height,
                                    std::min( capabilities.maxImageExtent.height,
                                              actual_extent.height ) );

    return actual_extent;
  }
}

#endif
