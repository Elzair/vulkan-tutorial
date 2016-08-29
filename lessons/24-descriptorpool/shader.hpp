#ifndef __SHADER_HPP__
#define __SHADER_HPP__

#include <fstream>
#include "base-includes.hpp"

static std::vector<char> readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

void createShaderModule( VkDevice device,
                         const std::vector<char>&  code,
                         VDeleter<VkShaderModule>& shaderModule)
{
  VkShaderModuleCreateInfo shaderCreateInfo = {};
  shaderCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.codeSize = code.size();
  shaderCreateInfo.pCode    = (uint32_t*) code.data();

  if ( vkCreateShaderModule( device, &shaderCreateInfo,
                             nullptr, &shaderModule ) != VK_SUCCESS )
  {
    throw std::runtime_error( "Failed to create shader module!" );
  }
}

#endif
