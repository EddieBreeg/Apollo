#pragma once

#include <PCH.hpp>
#include <asset/Asset.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPUTexture;

namespace brk::editor {

	bool ImportTexture2d(IAsset& out_texture);
}

namespace brk::rdr {
	class BRK_API Texture2D : public IAsset, public _internal::HandleWrapper<SDL_GPUTexture*>
	{
	public:
		Texture2D(const AssetMetadata& metadata)
			: IAsset(metadata)
		{}
		~Texture2D();

		static constexpr EAssetType AssetType = EAssetType::Texture2D;

	private:
		friend bool brk::editor::ImportTexture2d(IAsset&);
	};
} // namespace brk::rdr