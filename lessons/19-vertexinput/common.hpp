#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include "base-includes.hpp"
#include "deleter.hpp"
#include "vkextensions.hpp"
#include "vertex.hpp"

const int WIDTH  = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
  "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#endif
