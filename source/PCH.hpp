#pragma once

#include <cstdint>

#define INT_DECL(type)	using type = type##_t
INT_DECL(int8);
INT_DECL(int16);
INT_DECL(int32);
INT_DECL(int64);
INT_DECL(uint8);
INT_DECL(uint16);
INT_DECL(uint32);
INT_DECL(uint64);
#undef INT_DECL