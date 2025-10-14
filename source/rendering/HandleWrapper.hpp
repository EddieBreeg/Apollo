#pragma once

#include <core/Unassigned.hpp>

namespace apollo::rdr::_internal {
	template <class HandleType>
	struct HandleWrapper
	{
		HandleWrapper() = default;
		HandleWrapper(const HandleWrapper& other) = delete;

		HandleWrapper(HandleWrapper&& other)
			: m_Handle(other.m_Handle)
		{
			other.m_Handle = Unassigned<HandleType>;
		}

		void Swap(HandleWrapper& other) noexcept(std::is_nothrow_swappable_v<HandleType>)
		{
			std::swap(m_Handle, other.m_Handle);
		}

		HandleWrapper& operator=(const HandleWrapper&) = delete;
		HandleWrapper& operator=(HandleWrapper&& other) noexcept(noexcept(Swap(other)))
		{
			Swap(other);
			return *this;
		}

		[[nodiscard]] HandleType GetHandle() const noexcept { return m_Handle; }
		[[nodiscard]] operator bool() const noexcept { return m_Handle != Unassigned<HandleType>; }

	protected:
		using BaseType = HandleWrapper<HandleType>;

		HandleType m_Handle = Unassigned<HandleType>;
	};
} // namespace apollo::rdr::_internal