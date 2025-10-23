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
#include <rendering/Context.hpp>

namespace {
	apollo::EAssetLoadResult ValidateShaderStates(
		const apollo::rdr::GraphicsShader* vShader,
		const apollo::rdr::GraphicsShader* fShader)
	{
		const apollo::EAssetState s1 = vShader->GetState(), s2 = fShader->GetState();
		if (s1 == apollo::EAssetState::Invalid || s2 == apollo::EAssetState::Invalid)
			return apollo::EAssetLoadResult::Failure;

		if (s1 == apollo::EAssetState::Loaded && s2 == apollo::EAssetState::Loaded)
			return apollo::EAssetLoadResult::Success;

		return apollo::EAssetLoadResult::TryAgain;
	}

	apollo::EAssetLoadResult LoadMaterialJson(
		const apollo::AssetMetadata& metadata,
		apollo::AssetRef<apollo::rdr::VertexShader>& out_vShader,
		apollo::AssetRef<apollo::rdr::FragmentShader>& out_fShader,
		void*& out_handle)
	{
		using apollo::json::Converter;
		using namespace apollo;
		auto* manager = AssetManager::GetInstance();
		DEBUG_CHECK(manager)
		{
			APOLLO_LOG_CRITICAL(
				"Tried to load material but asset manager has not been initialised");
			return EAssetLoadResult::Failure;
		}

		std::ifstream file{ metadata.m_FilePath, std::ios::binary };
		if (!file.is_open())
		{
			APOLLO_LOG_ERROR(
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
			APOLLO_LOG_ERROR(
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
			APOLLO_LOG_ERROR("Failed to get vertex shader");
			return EAssetLoadResult::Failure;
		}
		if (!json::Visit(out_fShader, j, "fragmentShader"))
		{
			APOLLO_LOG_ERROR("Failed to get fragment shader");
			return EAssetLoadResult::Failure;
		}
		const EAssetLoadResult result = ValidateShaderStates(out_vShader.Get(), out_fShader.Get());
		switch (result)
		{
		case apollo::EAssetLoadResult::Failure: return result;
		case apollo::EAssetLoadResult::TryAgain:
		{ // #hack: temporary store the graphics pipeline description, we'll need it later
			auto* handle = new SDL_GPUGraphicsPipelineCreateInfo{ desc };
			auto* targetDesc = new SDL_GPUColorTargetDescription[desc.target_info.num_color_targets];
			for (uint32 i = 0; i < desc.target_info.num_color_targets; ++i)
			{
				targetDesc[i] = desc.target_info.color_target_descriptions[i];
			}
			handle->target_info.color_target_descriptions = targetDesc;
			out_handle = handle;
		}
			return EAssetLoadResult::TryAgain;
		default: break;
		}

		desc.vertex_shader = out_vShader->GetHandle();
		desc.fragment_shader = out_vShader->GetHandle();
		auto& device = rdr::Context::GetInstance()->GetDevice();

		out_handle = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), &desc);
		if (!out_handle)
		{
			APOLLO_LOG_ERROR("Failed to create graphics pipeline: {}", SDL_GetError());
			return EAssetLoadResult::Failure;
		}
		return EAssetLoadResult::Success;
	}
} // namespace

namespace apollo::editor {
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

		std::unique_ptr<SDL_GPUGraphicsPipelineCreateInfo> info{
			static_cast<SDL_GPUGraphicsPipelineCreateInfo*>(std::exchange(mat.m_Handle, nullptr)),
		};
		// make sure we also delete the target description array
		std::unique_ptr<const SDL_GPUColorTargetDescription[]> targetDesc{
			info->target_info.color_target_descriptions,
		};
		auto& device = rdr::Context::GetInstance()->GetDevice();
		info->vertex_shader = mat.m_VertShader->GetHandle();
		info->fragment_shader = mat.m_FragShader->GetHandle();

		mat.m_Handle = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), info.get());
		if (!mat.m_Handle)
		{
			APOLLO_LOG_ERROR("Failed to create graphics pipeline: {}", SDL_GetError());
			return EAssetLoadResult::Failure;
		}
		return EAssetLoadResult::Success;
	}
} // namespace apollo::editor