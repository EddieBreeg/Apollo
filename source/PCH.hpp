#pragma once

#include <BreakoutRuntimeExport.h>

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

#define INT_DECL(type) using type = type##_t
INT_DECL(int8);
INT_DECL(int16);
INT_DECL(int32);
INT_DECL(int64);
INT_DECL(uint8);
INT_DECL(uint16);
INT_DECL(uint32);
INT_DECL(uint64);
#undef INT_DECL

#ifdef _WIN32
extern "C" void* _alloca(size_t);
#define alloca _alloca
#else
extern "C" void* alloca(size_t);
#endif

#define Alloca(Type, n) static_cast<Type*>((n) * sizeof(Type))

#ifdef BRK_DEV
#define DEBUG_CHECK(expr) if (!(expr)) [[unlikely]]
#else
#define DEBUG_CHECK(expr) (void)(expr)
#endif