#include "AssetLoaders.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/ULID.hpp>
#include <core/ULIDFormatter.hpp>
#include <fstream>
#include <rendering/Material.hpp>
#include <rendering/Pipeline.hpp>
#include <rendering/Renderer.hpp>

namespace {
#ifdef BRK_DEV
	const char* GetStageName(brk::rdr::EShaderStage stage)
	{
		switch (stage)
		{
		case brk::rdr::EShaderStage::Fragment: return "Fragment";
		case brk::rdr::EShaderStage::Vertex: return "Vertex";
		default: return "Invalid";
		};
	}
#endif

	brk::EAssetLoadResult ValidateShaderStates(
		const brk::rdr::Shader* vShader,
		const brk::rdr::Shader* fShader)
	{
		const brk::EAssetState s1 = vShader->GetState(), s2 = fShader->GetState();
		if (s1 == brk::EAssetState::Invalid || s2 == brk::EAssetState::Invalid)
			return brk::EAssetLoadResult::Failure;

		if (s1 == brk::EAssetState::Loaded && s2 == brk::EAssetState::Loaded)
			return brk::EAssetLoadResult::Success;

		return brk::EAssetLoadResult::TryAgain;
	}

	bool ValidateShaderStages(const brk::rdr::Shader* vShader, const brk::rdr::Shader* fShader)
	{
		brk::rdr::EShaderStage stage = vShader->GetStage();
		if (stage != brk::rdr::EShaderStage::Vertex)
		{
			BRK_LOG_ERROR(
				"Shader {} has stage {}, expected Vertex",
				vShader->GetId(),
				GetStageName(stage));
			return false;
		}
		if ((stage = fShader->GetStage()) != brk::rdr::EShaderStage::Fragment)
		{
			BRK_LOG_ERROR(
				"Shader {} has stage {}, expected Fragment",
				fShader->GetId(),
				GetStageName(stage));
			return false;
		}
		return true;
	}

	brk::EAssetLoadResult LoadMaterialJson(
		const brk::AssetMetadata& metadata,
		brk::AssetRef<brk::rdr::Shader>& out_vShader,
		brk::AssetRef<brk::rdr::Shader>& out_fShader,
		void*& out_handle)
	{
		using brk::json::Converter;
		using namespace brk;
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

		SDL_GPUGraphicsPipelineCreateInfo desc = {};

		if (!Converter<SDL_GPUGraphicsPipelineCreateInfo>::FromJson(desc, j))
			return EAssetLoadResult::Failure;

		if (!json::Visit(out_vShader, j, "vertexShader"))
		{
			BRK_LOG_ERROR("Failed to get vertex shader");
			return EAssetLoadResult::Failure;
		}
		if (!json::Visit(out_fShader, j, "fragmentShader"))
		{
			BRK_LOG_ERROR("Failed to get fragment shader");
			return EAssetLoadResult::Failure;
		}
		const EAssetLoadResult result = ValidateShaderStates(out_vShader.Get(), out_fShader.Get());
		switch (result)
		{
		case brk::EAssetLoadResult::Failure: return result;
		case brk::EAssetLoadResult::TryAgain:
			// #hack: temporary store the graphics pipeline description, we'll need it later
			out_handle = new SDL_GPUGraphicsPipelineCreateInfo{ desc };
			return result;
		default: break;
		}

		if (!ValidateShaderStages(out_vShader.Get(), out_fShader.Get()))
			return EAssetLoadResult::Failure;

		desc.vertex_shader = out_vShader->GetHandle();
		desc.fragment_shader = out_vShader->GetHandle();
		auto& device = rdr::Renderer::GetInstance()->GetDevice();

		out_handle = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), &desc);
		if (!out_handle)
		{
			BRK_LOG_ERROR("Failed to create graphics pipeline: {}", SDL_GetError());
			return EAssetLoadResult::Failure;
		}
		return EAssetLoadResult::Success;
	}
} // namespace

namespace brk::editor {
	EAssetLoadResult LoadMaterial(IAsset& out_asset, const AssetMetadata& metadata)
	{
		auto& mat = dynamic_cast<rdr::Material&>(out_asset);

		if (!mat.m_VertShader && !mat.m_FragShader)
			return LoadMaterialJson(metadata, mat.m_VertShader, mat.m_FragShader, mat.m_Handle);

		const EAssetLoadResult result = ValidateShaderStates(
			mat.m_VertShader.Get(),
			mat.m_FragShader.Get());
		if (result != EAssetLoadResult::Success)
			return result;

		if (!ValidateShaderStages(mat.m_VertShader.Get(), mat.m_FragShader.Get()))
			return EAssetLoadResult::Failure;

		std::unique_ptr<SDL_GPUGraphicsPipelineCreateInfo> info{
			static_cast<SDL_GPUGraphicsPipelineCreateInfo*>(std::exchange(mat.m_Handle, nullptr)),
		};
		auto& device = rdr::Renderer::GetInstance()->GetDevice();
		info->vertex_shader = mat.m_VertShader->GetHandle();
		info->fragment_shader = mat.m_FragShader->GetHandle();

		mat.m_Handle = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), info.get());
		if (!mat.m_Handle)
		{
			BRK_LOG_ERROR("Failed to create graphics pipeline: {}", SDL_GetError());
			return EAssetLoadResult::Failure;
		}
		return EAssetLoadResult::Success;
	}
} // namespace brk::editor