#pragma once

/*! \file PCH.hpp
 Global pre-compiled header
 */

#include <ApolloRuntimeExport.h>

#include "core/Rectangle.hpp"
#include <concepts>
#include <core/Util.hpp>

/**
 \defgroup macros Macros
 @{
*/

#ifdef _WIN32
extern "C" void* _alloca(size_t);
#define alloca _alloca
#else
#include <alloca.h>
#endif

/*!
 \def Alloca
 Allocates `n` objects of type `Type` on the stack
 */
#define Alloca(Type, n) static_cast<Type*>(alloca((n) * sizeof(Type)))

/*!
 \def DEBUG_CHECK(expr)
 Checks the value of `cond` in dev mode only, and executes the code in the following scope if the
 expression evaluates to `false`
 \param expr: The expression to evaluate
 \note If APOLLO_DEV is not defined, this simply evaluates the expression without the if statement.
 */

#ifdef APOLLO_DEV
#define DEBUG_CHECK(expr) if (!(expr)) [[unlikely]]
#else
#define DEBUG_CHECK(expr)                                                                          \
	(void)(expr);                                                                                  \
	if constexpr (false)
#endif

/*!
 \def BIT(n)
 \returns A bit mask where only the nth (0-based) rightmost bit is set, i.e. #BIT(0) is 1
*/
#ifndef BIT
#define BIT(n) (1ull << (n))
#endif

/**
 \def STATIC_ARRAY_SIZE(arr)
 Evaluates the size of a static array
*/
#define STATIC_ARRAY_SIZE(arr) std::extent_v<decltype(arr)>

/** @} */

/// Single precision float rectangle
using RectF = apollo::Rectangle<float>;
/// Signed 32-bit int rectangle
using RectI32 = apollo::Rectangle<int32>;
/// Unsigned 32-bit int rectangle
using RectU32 = apollo::Rectangle<uint32>;

#include <core/Unassigned.hpp>