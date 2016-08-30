#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include "base-includes.hpp"
#include "deleter.hpp"
#include "memory.hpp"
#include "vkextensions.hpp"
//#include "vertex.hpp"

const int WIDTH  = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
  "VK_LAYER_LUNARG_standard_validation"
};

const std::string MODEL_PATH   = "chalet.obj";
const std::string TEXTURE_PATH = "chalet.jpg";

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#endif
