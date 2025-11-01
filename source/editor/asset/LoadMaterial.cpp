#include "AssetLoaders.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/ULID.hpp>
#include <fstream>
#include <rendering/Context.hpp>
#include <rendering/Material.hpp>
#include <rendering/Pipeline.hpp>

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
		if (!manager) [[unlikely]]
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

		if (!out_asset.IsLoadingDeferred())
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

	EAssetLoadResult LoadMaterialInstance(IAsset& out_asset, const AssetMetadata& metadata)
	{
		auto& instance = dynamic_cast<rdr::MaterialInstance&>(out_asset);
		if (instance.IsLoadingDeferred())
		{
			if (instance.m_Material->IsLoading())
				return EAssetLoadResult::TryAgain;
			return instance.m_Material->IsLoaded() ? EAssetLoadResult::Success
												   : EAssetLoadResult::Failure;
		}

		std::ifstream inFile{ metadata.m_FilePath, std::ios::binary };
		if (!inFile.is_open())
		{
			APOLLO_LOG_ERROR(
				"Failed to load material instance from {}: {}",
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}

		nlohmann::json json = nlohmann::json::parse(inFile, nullptr, false);
		if (json.is_discarded())
		{
			APOLLO_LOG_ERROR("Failed to parse {} as JSON", metadata.m_FilePath.string());
			return EAssetLoadResult::Failure;
		}

		ULID matId;
		if (!json::Visit(matId, json, "material"))
		{
			APOLLO_LOG_ERROR("Failed to get material ID for instance {}", metadata.m_Name);
			return EAssetLoadResult::Failure;
		}

		if (!matId) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Invalid material ID for instance {}", metadata.m_Name);
			return EAssetLoadResult::Failure;
		}

		AssetManager* manager = AssetManager::GetInstance();

		if (!manager) [[unlikely]]
		{
			return EAssetLoadResult::Failure;
		}

		const auto texturesIt = json.find("textures");
		if (texturesIt != json.end())
		{
		}

		struct MaterialCallback
		{
			rdr::MaterialInstance& instance;
			nlohmann::json m_ParamsDesc;

			void LoadConstantBlock(uint32 index, const rdr::ShaderConstantBlock& block)
			{
				instance.m_ConstantBlocks.m_BlockSizes[index] = block.m_Size;
				constexpr uint32 maxBlockSize = sizeof(rdr::MaterialInstance::ConstantBlockStorage);
				if (block.m_Size > maxBlockSize) [[unlikely]]
				{
					APOLLO_LOG_WARN(
						"Shader constant block {} has size {}, which is over the maximum limit of "
						"{}. Truncation will occur",
						index,
						block.m_Size,
						maxBlockSize);
					instance.m_ConstantBlocks.m_BlockSizes[index] = maxBlockSize;
				}

				if (index >= m_ParamsDesc.size())
					return;

				const nlohmann::json& blockDesc = m_ParamsDesc[index];
				if (!blockDesc.is_object())
				{
					APOLLO_LOG_ERROR(
						"Cann't parse shader constant block {} from JSON: not an object",
						index);
					return;
				}

				for (uint32 i = 0; i < block.m_NumMembers; ++i)
				{
					const auto& member = block.m_Members[i];
					LoadBlockMember(member, index, blockDesc);
				}
			}

#define VISIT_CONSTANT(type)                                                                       \
	json::Visit(                                                                                   \
		instance.GetFragmentConstant<type>(blockIndex, constant.m_Offset),                         \
		blockJson,                                                                                 \
		constant.m_Name,                                                                           \
		true);                                                                                     \
	break

			void LoadBlockMember(
				const rdr::ShaderConstant& constant,
				uint32 blockIndex,
				const nlohmann::json& blockJson)
			{
				bool result = true;
				switch (constant.m_Type)
				{
				case rdr::ShaderConstant::Float: result = VISIT_CONSTANT(float);
				case rdr::ShaderConstant::Float2: result = VISIT_CONSTANT(float2);
				case rdr::ShaderConstant::Float3: result = VISIT_CONSTANT(float3);
				case rdr::ShaderConstant::Float4: result = VISIT_CONSTANT(float4);
				case rdr::ShaderConstant::Int: result = VISIT_CONSTANT(int32);
				case rdr::ShaderConstant::Int2: result = VISIT_CONSTANT(glm::ivec2);
				case rdr::ShaderConstant::Int3: result = VISIT_CONSTANT(glm::ivec3);
				case rdr::ShaderConstant::Int4: result = VISIT_CONSTANT(glm::ivec4);
				case rdr::ShaderConstant::UInt: result = VISIT_CONSTANT(uint32);
				case rdr::ShaderConstant::UInt2: result = VISIT_CONSTANT(glm::uvec2);
				case rdr::ShaderConstant::UInt3: result = VISIT_CONSTANT(glm::uvec3);
				case rdr::ShaderConstant::UInt4: result = VISIT_CONSTANT(glm::uvec4);
				default:
					APOLLO_LOG_ERROR(
						"Failed to load parameter value for constant {} in block index {}: "
						"unsupported type",
						constant.m_Name,
						blockIndex);
					return;
				}
				if (!result)
				{
					APOLLO_LOG_ERROR(
						"Failed to get parse value for parameter {} in block {}",
						constant.m_Name,
						blockIndex);
				}
			}
#undef CONSTANT_TYPE_CASE

			void operator()(const IAsset&)
			{
				if (m_ParamsDesc.is_null() || !instance.m_Material->IsLoaded())
					return;

				if (!m_ParamsDesc.is_array())
				{
					APOLLO_LOG_ERROR(
						"Can't parse shader constant blocks from JSON for material instance: not "
						"an array");
					return;
				}

				const std::span fragConstantBlocks =
					instance.m_Material->GetFragmentShader()->GetParameterBlocks();

				for (uint32 i = 0; i < fragConstantBlocks.size(); ++i)
				{
					LoadConstantBlock(i, fragConstantBlocks[i]);
				}
			}
		} callback{ instance };

		if (!json::Visit(callback.m_ParamsDesc, json, "parameters", true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse material parameters from JSON for instance {}",
				metadata.m_Name);
			callback.m_ParamsDesc = nullptr;
		}

		instance.m_Material = manager->GetAsset<rdr::Material>(matId, std::move(callback));
		if (!instance.m_Material)
			return EAssetLoadResult::Failure;

		if (instance.m_Material->IsLoaded())
			return EAssetLoadResult::Success;
		return EAssetLoadResult::TryAgain;
	}
} // namespace apollo::editor