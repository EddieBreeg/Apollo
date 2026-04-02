#pragma once

#include <PCH.hpp>
#include <memory>

namespace apollo {
	/**
	* \brief Generic and opaque blob interface.

	This serves as a utility to allocate both a memory buffer, and a header with an interface to
	manage it
	*/
	class Blob
	{
	public:
		Blob(void* ptr, size_t size)
			: m_Ptr(static_cast<std::byte*>(ptr))
			, m_Size(size)
		{}
		Blob(const Blob&) = default;
		Blob& operator=(const Blob&) = default;

		template <class T, class... A>
		[[nodiscard]] static T* Allocate(size_t n, A&&... args)
			requires(requires(void* p, size_t n) {
				T{ p, n, std::forward<A>(args)... };
			} && std::is_base_of_v<Blob, T>)
		{
			T* const base = static_cast<T*>(::operator new(sizeof(T) + n));
			return new (base) T{ base + 1, n, std::forward<A>(args)... };
		}

		[[nodiscard]] static Blob* Allocate(size_t n)
		{
			std::byte* b = static_cast<std::byte*>(::operator new(n + sizeof(Blob)));
			return new (b) Blob{ b + sizeof(Blob), n };
		}

		void operator delete(void* ptr)
		{
			Blob* b = static_cast<Blob*>(ptr);
			::operator delete(ptr, b->GetAllocSize());
		}

		[[nodiscard]] size_t GetSize() const noexcept { return m_Size; }
		[[nodiscard]] void* GetPtr(size_t offset = 0) noexcept { return m_Ptr + offset; }
		[[nodiscard]] const void* GetPtr(size_t offset = 0) const noexcept
		{
			return m_Ptr + offset;
		}
		template <class T>
		[[nodiscard]] T* GetPtrAs(size_t index = 0) noexcept
		{
			return reinterpret_cast<T*>(m_Ptr) + index;
		}
		template <class T>
		[[nodiscard]] const T* GetPtrAs(size_t index = 0) const noexcept
		{
			return reinterpret_cast<const T*>(m_Ptr) + index;
		}

		virtual ~Blob() = default;

	private:
		size_t GetAllocSize() const noexcept
		{
			return m_Ptr - reinterpret_cast<const std::byte*>(this) + m_Size;
		}

	protected:
		std::byte* m_Ptr = nullptr;
		size_t m_Size = 0;
	};
} // namespace apollo

namespace std {
	unique_ptr(apollo::Blob*) -> unique_ptr<apollo::Blob>;
}