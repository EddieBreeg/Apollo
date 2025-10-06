#pragma once

#include <PCH.hpp>

/**
 * \file TypeInfo.hpp
 * Custom RTTI implementation. Currently, this kinda breaks down when crossing DLL boundaries...
 */

namespace brk {
	namespace _internal {
		struct BRK_API DefaultTypeIndexGenerator
		{
			static uint32 GetNext() noexcept;
		};
	} // namespace _internal

	template <class T, class G = _internal::DefaultTypeIndexGenerator>
	struct TypeIndex
	{
		[[nodiscard]] static uint32 GetValue() noexcept
		{
			static const uint32 index = G::GetNext();
			return index;
		}

		[[nodiscard]] operator uint32() const noexcept { return GetValue(); }
	};

	struct TypeInfo
	{
	private:
		BRK_API TypeInfo(uint32 index, uint32 size, uint32 alignment)
			: m_Index(index)
			, m_Size(size)
			, m_Alignment(alignment)
		{}

	public:
		template <class T>
		static const TypeInfo& Get() noexcept
		{
			if constexpr (!std::is_void_v<T>)
			{
				static const TypeInfo info{ TypeIndex<T>{}, sizeof(T), alignof(T) };
				return info;
			}
			else
			{
				static const TypeInfo info{ TypeIndex<void>{}, 0, 0 };
				return info;
			}
		}

		const uint32 m_Index, m_Size, m_Alignment;
	};

	[[nodiscard]] inline bool operator==(const TypeInfo& a, const TypeInfo& b) noexcept
	{
		return a.m_Index == b.m_Index;
	}

	[[nodiscard]] inline bool operator!=(const TypeInfo& a, const TypeInfo& b) noexcept
	{
		return a.m_Index != b.m_Index;
	}
} // namespace brk