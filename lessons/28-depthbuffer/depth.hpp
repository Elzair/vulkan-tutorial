#ifndef __DEPTH_HPP__
#define __DEPTH_HPP__

#include "base-includes.hpp"

VkFormat findSupportedFormat( VkPhysicalDevice             physical,
                              const std::vector<VkFormat>& candidates,
                              VkImageTiling                tiling,
                              VkFormatFeatureFlags         features )
{
  for (VkFormat format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  throw std::runtime_error( "Failed to find supported format!" );
}

VkFormat findDepthFormat( VkPhysicalDevice physical )
{
  return findSupportedFormat(
    physical,
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

#endif
