#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "Shader.hpp"
#include <asset/Asset.hpp>
#include <asset/AssetRef.hpp>
#include <core/Assert.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUGraphicsPipelineCreateInfo;

namespace apollo {
	struct AssetMetadata;
	enum class EAssetLoadResult : int8;
} // namespace apollo

namespace apollo::editor {
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata);
	EAssetLoadResult LoadMaterialInstance(IAsset& out_asset, const AssetMetadata& metadata);
} // namespace apollo::editor

namespace apollo::rdr {
	class Material : public IAsset, public _internal::HandleWrapper<void*>
	{
	public:
		using IAsset::IAsset;
		Material() = default;
		Material(Material&& other)
			: IAsset(other.m_Id)
			, BaseType(std::move(other))
			, m_VertShader(std::move(other.m_VertShader))
			, m_FragShader(std::move(other.m_FragShader))
		{}
		APOLLO_API ~Material();

		void Swap(Material& other) noexcept
		{
			BaseType::Swap(other);
			m_VertShader.Swap(other.m_VertShader);
			m_FragShader.Swap(other.m_FragShader);
			std::swap(m_Id, other.m_Id);
		}

		Material& operator=(Material&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		GET_ASSET_TYPE_IMPL(EAssetType::Material);

		[[nodiscard]] const VertexShader* GetVertexShader() const noexcept
		{
			return m_VertShader.Get();
		}
		[[nodiscard]] const FragmentShader* GetFragmentShader() const noexcept
		{
			return m_FragShader.Get();
		}

	private:
		friend EAssetLoadResult editor::LoadMaterial(
			IAsset& out_asset,
			const AssetMetadata& metadata);

		AssetRef<VertexShader> m_VertShader;
		AssetRef<FragmentShader> m_FragShader;
	};

	class MaterialInstance : public IAsset
	{
	public:
		using IAsset::IAsset;

		GET_ASSET_TYPE_IMPL(EAssetType::MaterialInstance);
		[[nodiscard]] const Material* GetMaterial() const noexcept { return m_Material.Get(); }

		template <class T>
		T& GetFragmentConstant(uint32 blockIndex, uint32 offset)
		{
			APOLLO_ASSERT(blockIndex < 4, "Block index {} is out of bounds", blockIndex);
			APOLLO_ASSERT(
				(offset + sizeof(T)) <= m_BlockSizes[blockIndex],
				"Offset {} is out of bounds",
				offset);
			auto& block = m_FragmentConstants[blockIndex].m_Buf;
			return *reinterpret_cast<T*>(block + offset);
		}

		template <class T>
		const T& GetFramgentConstant(uint32 blockIndex, uint32 offset) const
		{
			APOLLO_ASSERT(blockIndex < 4, "Block index {} is out of bounds", blockIndex);
			APOLLO_ASSERT(
				(offset + sizeof(T)) <= m_BlockSizes[blockIndex],
				"Offset {} is out of bounds",
				offset);
			auto& block = m_FragmentConstants[blockIndex].m_Buf;
			return *reinterpret_cast<T*>(block + offset);
		}

		APOLLO_API void PushFragmentConstants(SDL_GPUCommandBuffer* commandBuffer, uint32 blockIndex = 0);

	private:
		friend EAssetLoadResult editor::LoadMaterialInstance(
			IAsset& out_asset,
			const AssetMetadata& metadata);

		struct ConstantBlockStorage
		{
			alignas(std::max_align_t) uint8 m_Buf[128] = { 0 };
		};

		AssetRef<Material> m_Material;
		uint32 m_BlockSizes[4] = { 0 };
		ConstantBlockStorage m_FragmentConstants[4];
	};
} // namespace apollo::rdr