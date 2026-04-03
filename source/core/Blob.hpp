#pragma once

#include <PCH.hpp>
#include <memory>

namespace apollo {
	/**
	* \brief Generic and opaque blob interface.

	The Blob object itself is just a pointer and a size, and does \b not manage the
	lifetime of the buffer it points to. In that regard, it is akin to std::span. What the Blob
	class does however provide is a specialization allocation/deallocation interface to easily
	allocate both a buffer of the size you requested along with the actual Blob object, which is
	then constructed at the start of the memory region.
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

		/// \anchor BlobAllocate
		/** \name Allocate
		 * \brief Allocates a buffer big enough to hold both the size you requested AND the Blob
		 * object
		 * \param n: The size of the region you want to allocate
		 * \param args: Additional parameters to pass along to the blob constructor when using a
		 * custom type
		 * \tparam T: The type of header to use instead of the default Blob class
		 * \pre `T` must accept a pointer and a size as its first constructor parameters, and must
		 * inherit from Blob
		 * \returns A pointer to the header of the allocated block. The actual memory block of type
		 * \p n lies just beyond the header, and you can access it using the
		 \ref BlobGetPtr "GetPtr()" function
		 * @{ */
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
		/** @} */

		/**
		 * \brief De-allocates a buffer of memory. The ptr \b must have been allocated using \ref
		 * BlobAllocate "Allocate()"
		 */
		void operator delete(void* ptr)
		{
			Blob* b = static_cast<Blob*>(ptr);
			::operator delete(ptr, b->GetAllocSize());
		}

		[[nodiscard]] size_t GetSize() const noexcept { return m_Size; }
		/// \anchor BlobGetPtr
		/** \name GetPtr
		 * \brief Returns the stored pointer to the allocated buffer
		 * \param offset: Byte offset added to the address before returning
		 * @{ */
		[[nodiscard]] void* GetPtr(size_t offset = 0) noexcept { return m_Ptr + offset; }
		[[nodiscard]] const void* GetPtr(size_t offset = 0) const noexcept
		{
			return m_Ptr + offset;
		}
		/** @} */

		/** \name GetPtrAs
		 * \brief Same as \ref BlobGetPtr "GetPtr()" except the pointer is first cast
		 * using reinterpret_cast
		 * \tparam T: The destination type. No type checking is performed
		 * \param index: Offset added to the address \b after the cast
		 * \returns \code reinterpret_cast<T*>(m_Ptr) + index \endcode
		 * \important No type checking is performed, and behaviour is undefined if the memory buffer
		 * at that address doesn't contain an object of type compatible with `T`
		 * @{
		 */
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
		/** @}  */

		~Blob() = default;

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