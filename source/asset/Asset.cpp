#include "Asset.hpp"

namespace brk {
	IAsset::IAsset()
		: m_Id(ULID::Generate())
	{}
} // namespace brk