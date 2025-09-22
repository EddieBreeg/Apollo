#pragma once

#include "ULID.hpp"
#include <spdlog/fmt/bundled/core.h>

template <>
struct fmt::formatter<brk::ULID> : public fmt::formatter<fmt::basic_string_view<char>>
{
	fmt::appender format(const brk::ULID id, format_context& ctx) const
	{
		using Base = fmt::formatter<fmt::basic_string_view<char>>;
		char str[26];
		id.ToChars(str);
		return Base::format(basic_string_view<char>{ str, 26 }, ctx);
	}
};
