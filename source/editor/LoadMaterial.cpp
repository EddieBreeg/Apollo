#include "AssetImporters.hpp"
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/ULID.hpp>
#include <core/ULIDFormatter.hpp>
#include <fstream>
#include <rendering/Material.hpp>

namespace {
	const char* GetStageName(brk::rdr::EShaderStage stage)
	{
		switch (stage)
		{
		case brk::rdr::EShaderStage::Fragment: return "Fragment";
		case brk::rdr::EShaderStage::Vertex: return "Vertex";
		default: return "Invalid";
		};
	}
} // namespace

namespace brk::editor {
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata)
	{
		auto* manager = AssetManager::GetInstance();
		DEBUG_CHECK(manager)
		{
			BRK_LOG_CRITICAL("Tried to load material but asset manager has not been initialised");
			return EAssetLoadResult::Failure;
		}

		std::ifstream file{ metadata.m_FilePath, std::ios::binary };
		if (!file.is_open())
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}) from {}: {}",
				metadata.m_Name,
				metadata.m_Id,
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
		const nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
		if (j.is_discarded())
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}) from JSON",
				metadata.m_Name,
				metadata.m_Id);
			return EAssetLoadResult::Failure;
		}

		ULID vShaderId, fShaderId;
		if (!json::Visit(vShaderId, j, "vertexShader") || !vShaderId)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}) from JSON: missing/invalid vertex shader ID",
				metadata.m_Name,
				metadata.m_Id);
			return EAssetLoadResult::Failure;
		}
		if (!json::Visit(fShaderId, j, "fragmentShader") || !fShaderId)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}) from JSON: missing/invalid fragment shader ID",
				metadata.m_Name,
				metadata.m_Id);
			return EAssetLoadResult::Failure;
		}

		auto& mat = dynamic_cast<rdr::Material&>(out_asset);
		mat.m_VertShader = manager->GetAsset<rdr::Shader>(vShaderId);
		if (!mat.m_VertShader)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}): vertex shader {} was not found",
				metadata.m_Name,
				metadata.m_Id,
				vShaderId);
			return EAssetLoadResult::Failure;
		}
		mat.m_FragShader = manager->GetAsset<rdr::Shader>(fShaderId);
		if (!mat.m_FragShader)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}): fragment shader {} was not found",
				metadata.m_Name,
				metadata.m_Id,
				fShaderId);
			return EAssetLoadResult::Failure;
		}

		if (mat.m_VertShader->GetState() == EAssetState::Invalid ||
			mat.m_FragShader->GetState() == EAssetState::Invalid)
			return EAssetLoadResult::Failure;

		if (mat.m_VertShader->GetState() != EAssetState::Loaded ||
			mat.m_FragShader->GetState() != EAssetState::Loaded)
			return EAssetLoadResult::TryAgain;

		rdr::EShaderStage stage = mat.m_VertShader->GetStage();
		if (stage != rdr::EShaderStage::Vertex)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}): shader {} has stage {} instead of Vertex",
				metadata.m_Name,
				metadata.m_Id,
				vShaderId,
				GetStageName(stage));
			return EAssetLoadResult::Failure;
		}

		if ((stage = mat.m_FragShader->GetStage()) != rdr::EShaderStage::Fragment)
		{
			BRK_LOG_ERROR(
				"Failed to load material {}({}): shader {} has stage {} instead of Fragment",
				metadata.m_Name,
				metadata.m_Id,
				fShaderId,
				GetStageName(stage));
			return EAssetLoadResult::Failure;
		}

		return EAssetLoadResult::Success;
	}
} // namespace brk::editor