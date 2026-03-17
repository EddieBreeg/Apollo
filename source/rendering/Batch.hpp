#pragma once

#include <PCH.hpp>

#include "Buffer.hpp"
#include "Material.hpp"
#include <asset/AssetRef.hpp>

struct SDL_GPUCopyPass;
struct SDL_GPUGraphicsPipeline;

namespace apollo::rdr {
	template <class T>
	class Batch
	{
	public:
		Batch() = default;
		Batch(AssetRef<Material> material, uint32 maxSize)
			: m_Material(std::move(material))
			, m_Elems(std::make_unique<T[]>(maxSize))
			, m_Capacity(maxSize)
			, m_Buffer(rdr::EBufferFlags::GraphicsStorage, m_Capacity * sizeof(T))
		{}

		Batch(Batch&& other) noexcept
			: m_Material(std::move(other.m_Material))
			, m_Elems(std::move(other.m_Elems))
			, m_Capacity(other.m_Capacity)
			, m_Size(other.m_Size)
			, m_Dirty(other.m_Dirty)
		{
			other.m_Capacity = 0;
			other.m_Size = {};
			other.m_Dirty = false;
		}

		Batch& operator=(Batch&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		void Swap(Batch& other) noexcept
		{
			m_Material.Swap(other.m_Material);
			m_Buffer.Swap(other.m_Buffer);
			m_Elems.swap(other.m_Elems);
			std::swap(m_Capacity, other.m_Capacity);
			std::swap(m_Size, other.m_Size);
			std::swap(m_Dirty, other.m_Dirty);
		}

		void StartRecording() { m_Dirty = false; }

		void EndRecording(SDL_GPUCopyPass* copyPass)
		{
			if (!m_Dirty)
				return;
			m_Buffer.UploadData(copyPass, m_Elems.get(), m_Size.m_Current * sizeof(T));
			m_Dirty = false;
		}

		void Clear()
		{
			m_Size = {
				.m_Prev = m_Size.m_Current,
				.m_Current = 0,
			};
		}

		void Add(T&& elem)
			requires(requires(const T& a, const T& b) { { a != b }->std::same_as<bool>; })
		{
			if (m_Size.m_Current >= m_Capacity)
			{
				Reallocate();
				m_Dirty = true;
			}
			T* ptr = m_Elems.get() + m_Size.m_Current;
			if (++m_Size.m_Current > m_Size.m_Prev || elem != *ptr)
			{
				new (ptr) T{ std::move(elem) };
				m_Dirty = true;
			}
		}

		[[nodiscard]] const rdr::Buffer& GetBuffer() const noexcept { return m_Buffer; }
		[[nodiscard]] uint32 GetCount() const noexcept { return m_Size.m_Current; }
		[[nodiscard]] SDL_GPUGraphicsPipeline* GetPipeline() const noexcept
		{
			if (!m_Material)
				return nullptr;

			return m_Material->IsLoaded()
					   ? static_cast<SDL_GPUGraphicsPipeline*>(m_Material->GetHandle())
					   : nullptr;
		}

		T* begin() noexcept { return m_Elems.get(); }
		T* end() noexcept { return m_Elems.get() + m_Size.m_Current; }
		const T* begin() const noexcept { return m_Elems.get(); }
		const T* end() const noexcept { return m_Elems.get() + m_Size.m_Current; }

		T& operator[](uint32 i) { return m_Elems[i]; }
		const T& operator[](uint32 i) const { return m_Elems[i]; }

		operator bool() const noexcept { return m_Material && m_Buffer; }

	private:
		uint32 GrowCapacity(uint32 cap)
		{
			if (!cap)
				return 4;

			return (3 * cap - 1) / 2 + 1;
		}

		void Reallocate()
		{
			m_Capacity = GrowCapacity(m_Capacity);
			auto ptr = std::make_unique<T[]>(m_Capacity);
			for (uint32 i = 0; i < m_Size.m_Current; ++i)
			{
				ptr[i] = m_Elems[i];
			}
			m_Elems = std::move(ptr);
			m_Buffer = rdr::Buffer(rdr::EBufferFlags::GraphicsStorage, m_Capacity * sizeof(T));
		}

		AssetRef<Material> m_Material;
		std::unique_ptr<T[]> m_Elems;
		struct Size
		{
			uint32 m_Prev = 0;
			uint32 m_Current = 0;
		} m_Size;
		uint32 m_Capacity;
		Buffer m_Buffer;
		bool m_Dirty = false;
	};
} // namespace apollo::rdr