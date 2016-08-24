#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include "base-includes.hpp"
#include "deleter.hpp"
#include "vkextensions.hpp"

const int WIDTH  = 800;
const int HEIGHT = 600;

const std::vector<const char*> validation_layers = {
  "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

#endif