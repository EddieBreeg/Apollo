#include "AssetHelper.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <core/NumConv.hpp>
#include <core/ULID.hpp>
#include <fstream>
#include <rendering/Context.hpp>
#include <rendering/Material.hpp>
#include <rendering/Pipeline.hpp>

NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUSamplerAddressMode,
	{
		{ SDL_GPU_SAMPLERADDRESSMODE_REPEAT, "repeat" },
		{ SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT, "mirror" },
		{ SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE, "campToEdge" },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUSamplerMipmapMode,
	{
		{ SDL_GPU_SAMPLERMIPMAPMODE_NEAREST, "nearest" },
		{ SDL_GPU_SAMPLERMIPMAPMODE_LINEAR, "linear" },
	});
NLOHMANN_JSON_SERIALIZE_ENUM(
	SDL_GPUFilter,
	{
		{ SDL_GPU_FILTER_NEAREST, "nearest" },
		{ SDL_GPU_FILTER_LINEAR, "linear" },
	});

namespace {
#define VISIT_SAMPLER_PARAM(paramName, key)                                                        \
	if (!apollo::json::Visit(out_info.paramName, json, key, true))                                 \
	{                                                                                              \
		APOLLO_LOG_WARN("Failed to parse sampler parameter '" key "' from JSON");                  \
		return false;                                                                              \
	}

	bool Visit(SDL_GPUSamplerCreateInfo& out_info, const nlohmann::json& json)
	{
		VISIT_SAMPLER_PARAM(min_filter, "minFilter");
		VISIT_SAMPLER_PARAM(mag_filter, "magFilter");
		VISIT_SAMPLER_PARAM(mipmap_mode, "mipmapMode");
		VISIT_SAMPLER_PARAM(address_mode_u, "addressModeU");
		VISIT_SAMPLER_PARAM(address_mode_v, "addressModeV");
		VISIT_SAMPLER_PARAM(address_mode_w, "addressModeW");

		return true;
	}

#undef VISIT_SAMPLER_PARAM

	template <uint32 N>
	bool LoadTextures(
		apollo::AssetRef<apollo::rdr::Texture2D> (&out_textures)[N],
		SDL_GPUSampler* (&out_samplers)[N],
		uint32& out_numTextures,
		const nlohmann::json& json,
		apollo::rdr::GPUDevice& device)
	{
		using namespace apollo;

		if (!json.is_array())
		{
			APOLLO_LOG_ERROR("Failed to load textures from JSON: not an array");
			return 0;
		}
		uint32 nMax = apollo::NumCast<uint32>(json.size());

		if (nMax > N)
		{
			APOLLO_LOG_WARN(
				"JSON array size ({}) is over the maximum number of textures ({})",
				nMax,
				N);
			nMax = N;
		}

		out_numTextures = 0;
		while (out_numTextures < nMax)
		{
			const nlohmann::json& texDesc = json[out_numTextures];
			if (!json::Visit(out_textures[out_numTextures], texDesc, "id"))
			{
				APOLLO_LOG_ERROR("Failed to load texture {}: missing/invalid ID", out_numTextures);
				return false;
			}

			const auto it = texDesc.find("sampler");
			if (it == texDesc.end() || it->is_null())
			{
				++out_numTextures;
				continue;
			}

			SDL_GPUSamplerCreateInfo samplerInfo{
				.min_filter = SDL_GPU_FILTER_LINEAR,
				.mag_filter = SDL_GPU_FILTER_LINEAR,
				.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
				.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			};
			Visit(samplerInfo, *it);

			out_samplers[out_numTextures] = SDL_CreateGPUSampler(device.GetHandle(), &samplerInfo);
			++out_numTextures;
		}
		return true;
	}

#define VISIT_CONSTANT(type)                                                                       \
	apollo::json::Visit(                                                                           \
		instance.GetFragmentConstant<type>(blockIndex, constant.m_Offset),                         \
		blockJson,                                                                                 \
		constant.m_Name,                                                                           \
		true);                                                                                     \
	break

	void LoadBlockMember(
		apollo::rdr::MaterialInstance& instance,
		const apollo::rdr::ShaderConstant& constant,
		uint32 blockIndex,
		const nlohmann::json& blockJson)
	{
		using namespace apollo;
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
#undef VISIT_CONSTANT

	void LoadConstantBlock(
		const nlohmann::json& desc,
		apollo::rdr::MaterialInstance& instance,
		uint32 index,
		const apollo::rdr::ShaderConstantBlock& block)
	{
		if (index >= desc.size())
			return;

		const nlohmann::json& blockDesc = desc[index];
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
			LoadBlockMember(instance, member, index, blockDesc);
		}
	}
} // namespace

namespace apollo::editor {
	template <>
	AssetLoadTask AssetHelper<rdr::Material>::LoadAsync(IAsset& out_asset, const AssetMetadata& metadata)
	{
		auto& mat = dynamic_cast<rdr::Material&>(out_asset);

		auto* manager = IAssetManager::GetInstance();
		if (!manager) [[unlikely]]
		{
			APOLLO_LOG_CRITICAL(
				"Tried to load material but asset manager has not been initialised");
			co_return false;
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
			co_return false;
		}
		const nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
		if (j.is_discarded())
		{
			APOLLO_LOG_ERROR(
				"Failed to load material {}({}) from JSON",
				metadata.m_Name,
				metadata.m_Id);
			co_return false;
		}

		SDL_GPUGraphicsPipelineCreateInfo desc = {};
		using ConverterT = json::Converter<SDL_GPUGraphicsPipelineCreateInfo>;

		if (!ConverterT::FromJson(desc, j))
			co_return false;

		AssetRef<rdr::VertexShader> vShader;
		AssetRef<rdr::FragmentShader> fShader;
		if (!json::Visit(vShader, j, "vertexShader"))
		{
			APOLLO_LOG_ERROR("Failed to get vertex shader");
			co_return false;
		}
		if (!json::Visit(fShader, j, "fragmentShader"))
		{
			APOLLO_LOG_ERROR("Failed to get fragment shader");
			co_return false;
		}
		mat.m_VertShader = co_await std::move(vShader);
		if (!mat.m_VertShader->IsLoaded())
			co_return false;
		mat.m_FragShader = co_await std::move(fShader);
		if (!mat.m_FragShader->IsLoaded())
			co_return false;

		mat.m_MaterialKey |= desc.depth_stencil_state.enable_depth_write << 15;

		desc.vertex_shader = mat.m_VertShader->GetHandle();
		desc.fragment_shader = mat.m_FragShader->GetHandle();
		auto& device = rdr::Context::GetInstance()->GetDevice();

		mat.m_Handle = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), &desc);
		if (!mat.m_Handle)
		{
			APOLLO_LOG_ERROR("Failed to create graphics pipeline: {}", SDL_GetError());
			co_return false;
		}
		co_return true;
	}

	template <>
	AssetLoadTask AssetHelper<rdr::MaterialInstance>::LoadAsync(
		IAsset& out_asset,
		const AssetMetadata& metadata)
	{
		auto& instance = dynamic_cast<rdr::MaterialInstance&>(out_asset);

		std::ifstream inFile{ metadata.m_FilePath, std::ios::binary };
		if (!inFile.is_open())
		{
			APOLLO_LOG_ERROR(
				"Failed to load material instance from {}: {}",
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			co_return false;
		}

		nlohmann::json json = nlohmann::json::parse(inFile, nullptr, false);
		if (json.is_discarded())
		{
			APOLLO_LOG_ERROR("Failed to parse {} as JSON", metadata.m_FilePath.string());
			co_return false;
		}

		ULID matId;
		if (!json::Visit(matId, json, "material"))
		{
			APOLLO_LOG_ERROR("Failed to get material ID for instance {}", metadata.m_Name);
			co_return false;
		}

		if (!matId) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Invalid material ID for instance {}", metadata.m_Name);
			co_return false;
		}

		IAssetManager* manager = IAssetManager::GetInstance();

		if (!manager) [[unlikely]]
		{
			co_return false;
		}

		auto texturesIt = json.find("vertexTextures");
		auto* ctx = rdr::Context::GetInstance();
		if (texturesIt != json.end())
		{
			const bool res = LoadTextures(
				instance.m_VertexTextures.m_Textures,
				instance.m_VertexTextures.m_Samplers,
				instance.m_VertexTextures.m_NumTextures,
				*texturesIt,
				ctx->GetDevice());
			if (!res)
			{
				instance.Reset();
				co_return false;
			}
		}
		if ((texturesIt = json.find("fragmentTextures")) != json.end())
		{
			const bool res = LoadTextures(
				instance.m_FragmentTextures.m_Textures,
				instance.m_FragmentTextures.m_Samplers,
				instance.m_FragmentTextures.m_NumTextures,
				*texturesIt,
				ctx->GetDevice());
			if (!res)
			{
				instance.Reset();
				co_return false;
			}
		}

		nlohmann::json desc;

		if (!json::Visit(desc, json, "parameters", true))
		{
			APOLLO_LOG_ERROR(
				"Failed to parse material parameters from JSON for instance {}",
				metadata.m_Name);
			co_return false;
		}
		if (!desc.is_array())
		{
			APOLLO_LOG_ERROR(
				"Can't parse shader constant blocks from JSON for material instance: not "
				"an array");
			co_return false;
		}

		instance.m_Material = co_await manager->GetAsset<rdr::Material>(matId);
		if (!(instance.m_Material && instance.m_Material->IsLoaded()))
			co_return false;

		instance.m_Key = instance.m_Material->GenerateInstanceKey();

		const std::span
			fragConstantBlocks = instance.m_Material->GetFragmentShader()->GetParameterBlocks();
		instance.m_ConstantBlocks.Init(fragConstantBlocks);

		for (uint32 i = 0; i < fragConstantBlocks.size(); ++i)
		{
			LoadConstantBlock(desc, instance, i, fragConstantBlocks[i]);
		}

		co_return true;
	}
} // namespace apollo::editor