#ifndef __IMGVIEW_HPP__
#define __IMGVIEW_HPP__

#include "base-includes.hpp"

void createImageView( VkDevice               device,
                      VkImage                image,
                      VkFormat               format,
                      VkImageAspectFlags     aspectFlags,
                      VDeleter<VkImageView>& imageView )
{
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if ( vkCreateImageView( device, &viewInfo,
                          nullptr, &imageView ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to create texture image view!" );
  }
}

#endif
