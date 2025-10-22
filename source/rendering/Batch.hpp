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
			, m_NumElems(other.m_NumElems)
			, m_Dirty(other.m_Dirty)
		{
			other.m_Capacity = other.m_NumElems = 0;
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
			std::swap(m_NumElems, other.m_NumElems);
			std::swap(m_Dirty, other.m_Dirty);
		}

		void StartRecording() { m_Dirty = false; }

		void EndRecording(SDL_GPUCopyPass* copyPass)
		{
			if (!m_Dirty)
				return;
			m_Buffer.UploadData(copyPass, m_Elems.get(), m_NumElems * sizeof(T));
			m_Dirty = false;
		}

		void Clear() { m_NumElems = 0; }

		template <class... Args>
		void Add(Args&&... args)
		{
			if (m_NumElems >= m_Capacity)
			{
				Reallocate();
			}
			new (m_Elems.get() + m_NumElems++) T{ std::forward<Args>(args)... };
			m_Dirty = true;
		}

		[[nodiscard]] const rdr::Buffer& GetBuffer() const noexcept { return m_Buffer; }
		[[nodiscard]] uint32 GetCount() const noexcept { return m_NumElems; }
		[[nodiscard]] SDL_GPUGraphicsPipeline* GetPipeline() const noexcept
		{
			if (!m_Material)
				return nullptr;

			return m_Material->GetState() == EAssetState::Loaded
					   ? static_cast<SDL_GPUGraphicsPipeline*>(m_Material->GetHandle())
					   : nullptr;
		}

		T* begin() noexcept { return m_Elems.get(); }
		T* end() noexcept { return m_Elems.get() + m_NumElems; }
		const T* begin() const noexcept { return m_Elems.get(); }
		const T* end() const noexcept { return m_Elems.get() + m_NumElems; }

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
			for (uint32 i = 0; i < m_NumElems; ++i)
			{
				ptr[i] = m_Elems[i];
			}
			m_Elems = std::move(ptr);
			m_Buffer = rdr::Buffer(rdr::EBufferFlags::GraphicsStorage, m_Capacity * sizeof(T));
		}

		AssetRef<Material> m_Material;
		std::unique_ptr<T[]> m_Elems;
		uint32 m_NumElems = 0;
		uint32 m_Capacity;
		Buffer m_Buffer;
		bool m_Dirty = false;
	};
} // namespace apollo::rdr