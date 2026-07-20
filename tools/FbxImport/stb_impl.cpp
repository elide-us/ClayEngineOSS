// stb_impl.cpp - the single translation unit that compiles the stb
// implementations. Every other TU includes stb_image.h / stb_image_write.h
// WITHOUT the *_IMPLEMENTATION macros (declarations only), so the symbols live
// here exactly once. Keep this file tiny and dependency-free.

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// stb uses some CRT string functions the SDL check flags as "insecure" on MSVC.
// This is vendored public-domain code; silence the warnings for this TU only.
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "stb_image.h"
#include "stb_image_write.h"
