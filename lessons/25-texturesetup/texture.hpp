#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "base-includes.hpp"
#include "memory.hpp"
#include "buffer.hpp"

void createImage( VkPhysicalDevice          physical,
                  VkDevice                  device,
                  uint32_t                  width,
                  uint32_t                  height,
                  VkFormat                  format,
                  VkImageTiling             tiling,
                  VkImageUsageFlags         usage,
                  VkMemoryPropertyFlags     properties,
                  VDeleter<VkImage>&        image,
                  VDeleter<VkDeviceMemory>& imageMemory )
{
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  imageInfo.usage         = usage;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  if ( vkCreateImage( device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to create image!" );
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements( device, image, &memRequirements );

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType( physical,
                                              memRequirements.memoryTypeBits,
                                              properties );

  if ( vkAllocateMemory( device, &allocInfo,
                         nullptr, &imageMemory ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to allocate image memory!" );
  }

  vkBindImageMemory( device, image, imageMemory, 0 );
}

void transitionImageLayout( VkDevice      device,
                            VkQueue       queue,
                            VkCommandPool commandPool,
                            VkImage       image,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands( device, commandPool );

  endSingleTimeCommands( device, queue, commandPool, commandBuffer );

  // Create memory barrier
  VkImageMemoryBarrier barrier = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.srcAccessMask                   = 0; // TODO
  barrier.dstAccessMask                   = 0; // TODO

  // Create barrier masks
  if ( oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
       newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
  {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  }
  else if ( oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
  {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  }
  else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  }
  else
  {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier( commandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier );
}

void copyImage( VkDevice      device,
                VkQueue       queue,
                VkCommandPool commandPool,
                VkImage       srcImage,
                VkImage       dstImage,
                uint32_t      width,
                uint32_t      height )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands( device, commandPool );

  endSingleTimeCommands( device, queue, commandPool, commandBuffer );

  VkImageSubresourceLayers subResource = {};
  subResource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  subResource.baseArrayLayer = 0;
  subResource.mipLevel       = 0;
  subResource.layerCount     = 1;

  VkImageCopy region = {};
  region.srcSubresource = subResource;
  region.dstSubresource = subResource;
  region.srcOffset      = {0, 0, 0};
  region.dstOffset      = {0, 0, 0};
  region.extent.width   = width;
  region.extent.height  = height;
  region.extent.depth   = 1;

  vkCmdCopyImage( commandBuffer,
                  srcImage,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  dstImage,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  1,
                  &region );
}


#endif
