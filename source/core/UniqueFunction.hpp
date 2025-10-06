#pragma once

#include <PCH.hpp>

#include "Poly.hpp"
#include "StaticStorage.hpp"

namespace brk {
	template <class F>
	class UniqueFunction;

	template <class R, class... Args>
	class UniqueFunction<R(Args...)>
	{
		static constexpr uint32 StorageSize = 2 * sizeof(void*);

		template <class F>
		static constexpr bool IsInvocable = !std::is_same_v<F, UniqueFunction<R(Args...)>> &&
											std::is_invocable_r_v<R, F, Args...>;
		template <class F>
		static constexpr bool FitsStorage = sizeof(F) <= StorageSize &&
											alignof(F) <= alignof(std::max_align_t);

	public:
		UniqueFunction() = default;

		template <class F>
		explicit UniqueFunction(F&& func) noexcept(
			noexcept(m_Storage.Construct<std::decay_t<F>>(std::forward<F>(func))))
			requires(IsInvocable<std::decay_t<F>>&& FitsStorage<std::decay_t<F>>);

		template <class F>
		explicit UniqueFunction(F&& func)
			requires(IsInvocable<std::decay_t<F>> && !FitsStorage<std::decay_t<F>>);

		UniqueFunction(UniqueFunction&& other) noexcept;
		UniqueFunction& operator=(UniqueFunction&& other) noexcept;

		constexpr operator bool() const noexcept { return m_VTable; }

		R operator()(auto&&... args)
		{
			return m_VTable.m_Invoke(GetPtr(), std::forward<decltype(args)>(args)...);
		}

		void Swap(UniqueFunction& other) noexcept;

		~UniqueFunction();

	private:
		struct VTable
		{
			void (*m_Destroy)(void*) = nullptr;
			R (*m_Invoke)(void*, Args...) = nullptr;
			uint32 m_Size = 0;
			uint32 m_Alignment = 0;

			constexpr operator bool() const noexcept { return m_Destroy && m_Invoke; }
		} m_VTable;

		bool IsSmall() const noexcept
		{
			return m_VTable.m_Size <= StorageSize &&
				   m_VTable.m_Alignment <= alignof(decltype(m_Storage));
		}
		void* GetPtr() noexcept { return IsSmall() ? m_Storage.m_Buf : m_Ptr; }

		union {
			StaticStorage<2 * sizeof(void*)> m_Storage;
			void* m_Ptr = nullptr;
		};
	};

	template <class R, class... Args>
	UniqueFunction<R(Args...)>::~UniqueFunction()
	{
		if (m_VTable.m_Destroy)
			m_VTable.m_Destroy(GetPtr());
	}

	template <class R, class... Args>
	template <class F>
	UniqueFunction<R(Args...)>::UniqueFunction(F&& func) noexcept(
		noexcept(m_Storage.Construct<std::decay_t<F>>(std::forward<F>(func))))
		requires(IsInvocable<std::decay_t<F>>&& FitsStorage<std::decay_t<F>>)
		: m_VTable{
			&poly::Destroy<std::decay_t<F>>,
			&poly::Invoke<std::decay_t<F>, Args...>,
			sizeof(F),
			alignof(F),
		}
	{
		m_Storage.Construct<std::decay_t<F>>(std::forward<F>(func));
	}

	template <class R, class... Args>
	template <class F>
	UniqueFunction<R(Args...)>::UniqueFunction(F&& func)
		requires(IsInvocable<std::decay_t<F>> && !FitsStorage<std::decay_t<F>>)
		: m_VTable{
			&poly::Delete<std::decay_t<F>>,
			&poly::Invoke<std::decay_t<F>, Args...>,
			sizeof(F),
			alignof(F),
		}
	{
		using FuncType = std::decay_t<F>;
		m_Ptr = new FuncType{ std::forward<F>(func) };
	}

	template <class R, class... Args>
	UniqueFunction<R(Args...)>::UniqueFunction(UniqueFunction&& other) noexcept
		: m_VTable(other.m_VTable)
		, m_Storage(other.m_Storage)
	{
		other.m_VTable = {};
	}

	template <class R, class... Args>
	UniqueFunction<R(Args...)>& UniqueFunction<R(Args...)>::operator=(
		UniqueFunction&& other) noexcept
	{
		Swap(other);
		return *this;
	}

	template <class R, class... Args>
	void UniqueFunction<R(Args...)>::Swap(UniqueFunction& other) noexcept
	{
		m_Storage.Swap(other.m_Storage);
		std::swap(m_VTable, other.m_VTable);
	}
} // namespace brk