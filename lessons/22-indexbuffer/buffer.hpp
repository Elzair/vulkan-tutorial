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
                   VDeleter<VkDeviceMemory>& buffer_memory )
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = size;
    buffer_info.usage       = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( device, &buffer_info,
                         nullptr, &buffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create buffer!" );
    }

    VkMemoryRequirements memreqs;
    vkGetBufferMemoryRequirements( device, buffer, &memreqs );

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = memreqs.size;
    alloc_info.memoryTypeIndex = findMemoryType( physical,
                                                 memreqs.memoryTypeBits,
                                                 props );

    if ( vkAllocateMemory( device, &alloc_info,
                           nullptr, &buffer_memory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to allocate buffer memory!" );
    }

    vkBindBufferMemory( device, buffer, buffer_memory, 0 );
}

void copyBuffer( VkDevice      device,
                 VkQueue       queue,
                 VkCommandPool command_pool,
                 VkBuffer      src_buffer,
                 VkBuffer      dst_buffer,
                 VkDeviceSize  size )
{
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool        = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers( device, &alloc_info, &command_buffer );

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer( command_buffer, &begin_info );

  VkBufferCopy copy_region = {};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size      = size;
  vkCmdCopyBuffer( command_buffer, src_buffer, dst_buffer, 1, &copy_region );

  vkEndCommandBuffer( command_buffer );

  VkSubmitInfo submitInfo = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &command_buffer;

  vkQueueSubmit( queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( queue );

  vkFreeCommandBuffers( device, command_pool, 1, &command_buffer );
}

#endif
