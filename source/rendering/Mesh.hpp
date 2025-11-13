#pragma once

#include <PCH.hpp>

#include "Buffer.hpp"
#include <asset/Asset.hpp>

namespace apollo {
	enum class EAssetLoadResult : int8;
}

namespace apollo::editor {
		template <class>
	struct AssetHelper;
}

namespace apollo::rdr {
	class Mesh : public IAsset
	{
	public:
		using IAsset::IAsset;

		GET_ASSET_TYPE_IMPL(EAssetType::Mesh);

		[[nodiscard]] const Buffer& GetVertexBuffer() const noexcept { return m_VBuffer; }
		[[nodiscard]] const Buffer& GetIndexBuffer() const noexcept { return m_IBuffer; }
		[[nodiscard]] uint32 GetNumVertices() const noexcept { return m_NumVertices; }
		[[nodiscard]] uint32 GetNumIndices() const noexcept { return m_NumIndices; }

	private:
		Buffer m_VBuffer;
		Buffer m_IBuffer;
		uint32 m_NumVertices = 0;
		uint32 m_NumIndices = 0;

		friend struct editor::AssetHelper<Mesh>;
	};
} // namespace apollo::rdr