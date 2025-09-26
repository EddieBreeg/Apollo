#pragma once

#include <PCH.hpp>
#include <asset/Asset.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPUTexture;

namespace brk::editor {

	bool ImportTexture2d(IAsset& out_texture, const AssetMetadata& metadata);
}

namespace brk::rdr {
	class BRK_API Texture2D : public IAsset, public _internal::HandleWrapper<SDL_GPUTexture*>
	{
	public:
		Texture2D() = default;
		Texture2D(const ULID& id)
			: IAsset(id)
		{}

		~Texture2D();

		static constexpr EAssetType AssetType = EAssetType::Texture2D;
		GET_ASSET_TYPE_IMPL();

	private:
		friend bool brk::editor::ImportTexture2d(IAsset&, const AssetMetadata&);
	};
} // namespace brk::rdr