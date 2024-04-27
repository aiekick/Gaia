#include <Gaia/gaia.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#if defined(__clang__) || defined(__GNUC__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef STB_IMAGE_INCLUDE
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include STB_IMAGE_INCLUDE
#endif  // STB_IMAGE_INCLUDE

#ifdef STB_IMAGE_RESIZE_INCLUDE
#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#endif
#include STB_IMAGE_RESIZE_INCLUDE
#endif  // STB_IMAGE_RESIZE_INCLUDE

#ifdef STB_IMAGE_WRITE_INCLUDE
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include STB_IMAGE_WRITE_INCLUDE
#endif  // STB_IMAGE_WRITE_INCLUDE

#if defined(__clang__) || defined(__GNUC__)
#pragma clang diagnostic pop
#endif