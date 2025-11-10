#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include <asset/Asset.hpp>
#include <asset/AssetRef.hpp>
#include <core/Assert.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUGraphicsPipelineCreateInfo;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

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
		APOLLO_API ~MaterialInstance();
		APOLLO_API void Reset();

		static constexpr uint32 s_MaxTextures = 16;

		GET_ASSET_TYPE_IMPL(EAssetType::MaterialInstance);
		[[nodiscard]] const Material* GetMaterial() const noexcept { return m_Material.Get(); }

		template <class T>
		T& GetFragmentConstant(uint32 blockIndex, uint32 offset)
		{
			APOLLO_ASSERT(blockIndex < 4, "Block index {} is out of bounds", blockIndex);
			APOLLO_ASSERT(
				(offset + sizeof(T)) <= m_ConstantBlocks.m_BlockSizes[blockIndex],
				"Offset {} is out of bounds",
				offset);
			return *m_ConstantBlocks.m_FragmentConstants[blockIndex].GetAs<T>(offset);
		}

		template <class T>
		const T& GetFramgentConstant(uint32 blockIndex, uint32 offset) const
		{
			APOLLO_ASSERT(blockIndex < 4, "Block index {} is out of bounds", blockIndex);
			APOLLO_ASSERT(
				(offset + sizeof(T)) <= m_ConstantBlocks.m_BlockSizes[blockIndex],
				"Offset {} is out of bounds",
				offset);
			return *m_ConstantBlocks.m_FragmentConstants[blockIndex].GetAs<T>(offset);
		}

		APOLLO_API void PushFragmentConstants(
			SDL_GPUCommandBuffer* commandBuffer,
			uint32 blockIndex = 0) const;

		APOLLO_API void Bind(SDL_GPURenderPass* renderPass) const;

	private:
		friend EAssetLoadResult editor::LoadMaterialInstance(
			IAsset& out_asset,
			const AssetMetadata& metadata);

		AssetRef<Material> m_Material;
		struct
		{
			uint32 m_BlockSizes[4] = { 0 };
			ShaderConstantStorage m_FragmentConstants[4];
		} m_ConstantBlocks;

		struct TextureStorage
		{
			uint32 m_NumTextures = 0;
			AssetRef<Texture2D> m_Textures[16] = {};
			SDL_GPUSampler* m_Samplers[16] = { nullptr };
		} m_VertexTextures, m_FragmentTextures;
	};
} // namespace apollo::rdr