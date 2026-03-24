#pragma once

#include <PCH.hpp>
#include <asset/AssetManager.hpp>

/** \file Manager.hpp */

namespace apollo::editor {
	/**
	 * \brief Editor specific asset manager
	 */
	class AssetManager final : public IAssetManager
	{
	public:
		AssetManager(
			const std::filesystem::path& path,
			rdr::GPUDevice& device,
			mt::ThreadPool& threadPool);

		bool ImportMetadataBank() override;

		void RequestReload(IAsset& asset);

	protected:
		const AssetTypeInfo& GetTypeInfo(EAssetType type) const override;
	};
} // namespace apollo::editor