#include <Gaia/gaia.h>

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
#endif // STB_IMAGE_WRITE_INCLUDE
