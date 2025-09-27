#pragma once

#include <BreakoutRuntimeExport.h>

#include <concepts>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
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

using float2 = glm::vec<2, float>;
using float3 = glm::vec<3, float>;
using float4 = glm::vec<4, float>;

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

#ifndef BIT
#define BIT(n) (1ull << (n))
#endif

#define STATIC_ARRAY_SIZE(arr) std::extent_v<decltype(arr)>