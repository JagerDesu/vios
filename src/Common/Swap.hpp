#pragma once

#include <SDL2/SDL_endian.h>

#define VIOS_LITTLE_ENDIAN 1
#define VIOS_BIG_ENDIAN 2

#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
#define VIOS_BYTE_ORDER VIOS_LITTLE_ENDIAN
#else
#define VIOS_BYTE_ORDER VIOS_BIG_ENDIAN
#endif

namespace Common {
#if (VIOS_BYTE_ORDER == VIOS_LITTLE_ENDIAN)
using uint64_le_t = uint64_t;
using uint32_le_t = uint32_t;
using uint16_le_t = uint16_t;
#endif

using uint8_le_t = uint8_t;
using uint8_be_t = uint8_t;
}