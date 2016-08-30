#ifndef __UBO_HPP__
#define __UBO_HPP__

#include <glm/glm.hpp>

struct UniformBufferObject
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

#endif
