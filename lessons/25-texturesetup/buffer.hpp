#ifndef __BUFFER_HPP__
#define __BUFFER_HPP__

#include "base-includes.hpp"
#include "memory.hpp"

void createBuffer( VkDevice                  device,
                   VkPhysicalDevice          physical,
                   VkDeviceSize              size,
                   VkBufferUsageFlags        usage,
                   VkMemoryPropertyFlags     props,
                   VDeleter<VkBuffer>&       buffer,
                   VDeleter<VkDeviceMemory>& bufferMemory )
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = size;
  bufferInfo.usage       = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ( vkCreateBuffer( device, &bufferInfo,
                       nullptr, &buffer ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to create buffer!" );
  }

  VkMemoryRequirements memreqs;
  vkGetBufferMemoryRequirements( device, buffer, &memreqs );

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memreqs.size;
  allocInfo.memoryTypeIndex = findMemoryType( physical,
                                              memreqs.memoryTypeBits,
                                              props );

  if ( vkAllocateMemory( device, &allocInfo,
                         nullptr, &bufferMemory ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to allocate buffer memory!" );
  }

  vkBindBufferMemory( device, buffer, bufferMemory, 0 );
}

VkCommandBuffer beginSingleTimeCommands( VkDevice        device,
                                         VkCommandPool   commandPool)
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers( device, &allocInfo, &commandBuffer );

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer( commandBuffer, &beginInfo );

  return commandBuffer;
}

void endSingleTimeCommands( VkDevice        device,
                            VkQueue         queue,
                            VkCommandPool   commandPool,
                            VkCommandBuffer commandBuffer )
{
  vkEndCommandBuffer( commandBuffer );

  VkSubmitInfo submitInfo = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit( queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( queue );

  vkFreeCommandBuffers( device, commandPool, 1, &commandBuffer );
}

void copyBuffer( VkDevice      device,
                 VkQueue       queue,
                 VkCommandPool commandPool,
                 VkBuffer      srcBuffer,
                 VkBuffer      dstBuffer,
                 VkDeviceSize  size )
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands( device, commandPool );

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = size;
  vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

  endSingleTimeCommands( device, queue, commandPool, commandBuffer );
}

#endif
