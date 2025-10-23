#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include "Shader.hpp"
#include <asset/Asset.hpp>
#include <asset/AssetRef.hpp>

struct SDL_GPUGraphicsPipeline;
struct SDL_GPUGraphicsPipelineCreateInfo;

namespace apollo {
	struct AssetMetadata;
	enum class EAssetLoadResult : int8;
} // namespace apollo

namespace apollo::editor {
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata);
}

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

		[[nodiscard]] const VertexShader* GetVertexShader() const noexcept { return m_VertShader.Get(); }
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
} // namespace apollo::rdr