#pragma once

#include <PCH.hpp>
#include <asset/AssetManager.hpp>

namespace apollo::editor {
	class AssetManager final : public IAssetManager
	{
	public:
		AssetManager(
			const std::filesystem::path& path,
			rdr::GPUDevice& device,
			mt::ThreadPool& threadPool);

		bool ImportMetadataBank() override;

	protected:
		const AssetTypeInfo& GetTypeInfo(EAssetType type) const override;
	};
} // namespace apollo::editor