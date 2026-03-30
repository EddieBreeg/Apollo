#pragma once

#include <charconv>
#include <span>
#include <string_view>
#include <type_traits>

namespace argp {
	template <size_t N>
	struct CharArray
	{
		consteval CharArray(const char (&arr)[N])
		{
			char* it = m_Arr;
			for (const char x : arr)
				*it++ = x;
		}
		[[nodiscard]] constexpr operator std::string_view() const noexcept
		{
			return std::string_view{ m_Arr, N - 1 };
		}

		char m_Arr[N];
	};

	template <class T>
	bool ParseValue(T& out_val, std::string_view str) noexcept(noexcept(out_val = str))
		requires(requires { out_val = str; })
	{
		out_val = str;
		return true;
	}
	template <class T>
	bool ParseValue(T& out_val, std::string_view str) requires(std::is_arithmetic_v<T>)
	{
		if (str.empty())
			return false;

		const auto res = std::from_chars(str.data(), str.data() + str.size(), out_val);
		return res.ec == std::errc{};
	}
	inline bool ParseValue(const char*& out_val, const char* arg)
	{
		out_val = arg;
		return true;
	}

	struct MissingArgumentError
	{
		std::string_view m_Name;
	};

	struct InvalidValueError
	{
		std::string_view m_Name;
		std::string_view m_Value;
	};

	struct UnknownArgumentError
	{
		std::string_view m_Arg;
	};

	template <class T, size_t N>
	struct PositionalArgument;

	template <class T, class S, size_t N>
	struct PositionalArgument<T S::*, N>
	{
		T S::* m_Ptr;
		CharArray<N> m_Name;

		void TryParse(S& out_settings, std::span<const char* const>& args) const
		{
			if (args.empty())
				throw MissingArgumentError{ m_Name };

			if (!ParseValue(out_settings.*m_Ptr, args[0]))
				throw InvalidValueError{ m_Name, args[0] };

			args = args.subspan(1);
		}
	};

	template <class T, class S, size_t N>
	PositionalArgument(T S::* ptr, const char (&name)[N]) -> PositionalArgument<T S::*, N>;

	template <class M, size_t N>
	struct NamedArgument;

	template <class T, class S, size_t N>
	struct NamedArgument<T S::*, N>
	{
		T S::* m_Ptr;
		CharArray<N> m_Name;

		bool TryParse(S& out_settings, std::span<const char* const>& args) const
		{
			if (args.empty() || args[0] != (std::string_view)m_Name)
				return false;

			if (args.size() < 2)
				throw MissingArgumentError{ m_Name };

			if (!ParseValue(out_settings.*m_Ptr, args[1]))
				throw InvalidValueError{ m_Name, args[1] };

			args = args.subspan(2);
			return true;
		}
	};

	template <class S, size_t N>
	struct NamedArgument<bool S::*, N>
	{
		bool S::* m_Ptr;
		CharArray<N> m_Name;

		bool TryParse(S& out_settings, std::span<const char* const>& args) const
		{
			if (args.empty() || args[0] != (std::string_view)m_Name)
				return false;

			out_settings.*m_Ptr = true;
			args = args.subspan(1);
			return true;
		}
	};

	template <auto... T>
	struct ArgList;

	template <class S, class... T, size_t... N, PositionalArgument<T S::*, N>... Args>
	struct ArgList<Args...>
	{
		static constexpr size_t Size = sizeof...(Args);

		static std::span<const char* const> Parse(S& out_settings, std::span<const char* const> args)
		{
			((Args.TryParse(out_settings, args)), ...);
			return args;
		}
	};

	template <class S, class... T, size_t... N, NamedArgument<T S::*, N>... Args>
	struct ArgList<Args...>
	{
		static constexpr size_t Size = sizeof...(Args);

		static void Parse(S& out_settings, std::span<const char* const> args)
		{
			while (args.size())
			{
				bool found = false;
				((found = found ? true : found || Args.TryParse(out_settings, args)), ...);

				if (!found)
					throw UnknownArgumentError{ args[0] };
			}
		}
	};

	template <class T, class S, size_t N>
	NamedArgument(T S::* ptr, const char (&str)[N]) -> NamedArgument<T S::*, N>;
} // namespace argp