#pragma once

#include <PCH.hpp>

#include "Log.hpp"

#ifdef BRK_DEV
namespace brk {
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
} // namespace brk

#define BRK_ASSERT(expr, ...)                                                                      \
	brk::_internal::AssertImpl(                                                                    \
		!!(expr),                                                                                  \
		spdlog::source_loc{ __FILE__, __LINE__, __func__ },                                        \
		__VA_ARGS__)

#define DEBUG_BREAK()                                                                              \
	throw brk::_internal::BreakException                                                           \
	{                                                                                              \
		__FILE__, __func__, __LINE__                                                               \
	}

#else
#define BRK_ASSERT(expr, ...) (void)(expr)
#define DEBUG_BREAK()		 (void)0
#endif