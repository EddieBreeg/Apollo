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
	template <class>
	struct AssetHelper;
} // namespace apollo::editor

namespace apollo::rdr {
	struct MaterialInstanceKey
	{
		MaterialInstanceKey() = default;
		MaterialInstanceKey(uint32 val) noexcept
			: m_Value(val)
		{}

		MaterialInstanceKey(const MaterialInstanceKey&) noexcept = default;
		MaterialInstanceKey& operator=(const MaterialInstanceKey&) noexcept = default;

		template <std::unsigned_integral UInt>
		[[nodiscard]] operator UInt() const noexcept
		{
			return m_Value & ~s_DepthWriteBit;
		}

		[[nodiscard]] bool WritesToDepthBuffer() const noexcept
		{
			return m_Value & s_DepthWriteBit;
		}

		[[nodiscard]] uint16 GetMaterialIndex() const noexcept
		{
			return (m_Value >> 15) & s_IndexMask;
		}
		[[nodiscard]] uint16 GetInstanceIndex() const noexcept { return m_Value & s_IndexMask; }

	private:
		static constexpr uint32 s_DepthWriteBit = BIT(30);
		static constexpr uint16 s_IndexMask = 0x7FFF;

		uint32 m_Value = UINT32_MAX;
	};

	class Material : public IAsset, public _internal::HandleWrapper<void*>
	{
	public:
		APOLLO_API Material(const ULID& id = ULID::Generate());
		APOLLO_API ~Material();

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

		[[nodiscard]] MaterialInstanceKey GenerateInstanceKey() noexcept
		{
			return static_cast<MaterialInstanceKey>(
				(static_cast<uint32>(m_MaterialKey) << 15) | (m_InstanceKey++ & 0x7FFF));
		}

	private:
		friend struct editor::AssetHelper<Material>;

		AssetRef<VertexShader> m_VertShader;
		AssetRef<FragmentShader> m_FragShader;
		uint16 m_MaterialKey;
		std::atomic_uint16_t m_InstanceKey = 0;
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

		[[nodiscard]] MaterialInstanceKey GetKey() const noexcept { return m_Key; }

	private:
		friend struct editor::AssetHelper<MaterialInstance>;

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

		MaterialInstanceKey m_Key;
	};
} // namespace apollo::rdr