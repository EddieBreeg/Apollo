#pragma once

#include <ApolloRuntimeExport.h>

#include "core/Rectangle.hpp"
#include <concepts>
#include <core/Util.hpp>

#ifdef _WIN32
extern "C" void* _alloca(size_t);
#define alloca _alloca
#else
#include <alloca.h>
#endif

#define Alloca(Type, n) static_cast<Type*>(alloca((n) * sizeof(Type)))

#ifdef APOLLO_DEV
#define DEBUG_CHECK(expr) if (!(expr)) [[unlikely]]
#else
#define DEBUG_CHECK(expr)                                                                          \
	(void)(expr);                                                                                  \
	if constexpr (false)
#endif

#ifndef BIT
#define BIT(n) (1ull << (n))
#endif

#define STATIC_ARRAY_SIZE(arr) std::extent_v<decltype(arr)>

using RectF = apollo::Rectangle<float>;
using RectI32 = apollo::Rectangle<int32>;
using RectU32 = apollo::Rectangle<uint32>;