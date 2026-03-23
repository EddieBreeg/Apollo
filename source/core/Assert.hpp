#pragma once

#include <PCH.hpp>

/** \file Assert.hpp */

#include "Log.hpp"

#ifdef APOLLO_DEV
namespace apollo {
	namespace _internal {
		struct BreakException
		{
			const char* m_File = nullptr;
			const char* m_FuncName = nullptr;
			int32 m_Line = -1;
		};

		template <class... A>
		void AssertImpl(
			const bool cond,
			spdlog::source_loc loc,
			spdlog::format_string_t<A...> fmt,
			A&&... args)
		{
			if (cond) [[likely]]
				return;

			spdlog::log(loc, spdlog::level::critical, fmt, std::forward<A>(args)...);
			throw _internal::BreakException{ loc.filename, loc.funcname, loc.line };
		}
	} // namespace _internal
} // namespace apollo

/**
 \addtogroup macros
 @{
*/

/**
 * \def APOLLO_ASSERT(expr, ...)
 * If APOLLO_DEV is defined, asserts that the provided expression evaluates to true, and
 * raises an exception if this condition isn't met. The other arguments are passed to fmt::format().
 * \note If APOLLO_DEV is not defined, simply evaluates the expression without any checks
 */
#define APOLLO_ASSERT(expr, ...)                                                                   \
	apollo::_internal::AssertImpl(                                                                 \
		!!(expr),                                                                                  \
		spdlog::source_loc{ __FILE__, __LINE__, __func__ },                                        \
		__VA_ARGS__)

/**
 * \def DEBUG_BREAK()
 * If APOLLO_DEV is defined, throws an exception to pause the debugger.
 * \note If APOLLO_DEV is not defined, does nothing.
 */
#define DEBUG_BREAK()                                                                              \
	throw apollo::_internal::BreakException                                                        \
	{                                                                                              \
		__FILE__, __func__, __LINE__                                                               \
	}

#else
#define APOLLO_ASSERT(expr, ...) (void)(expr)
#define DEBUG_BREAK()			 (void)0
#endif

/** @} */