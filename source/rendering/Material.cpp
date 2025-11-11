#include "Material.hpp"
#include "Context.hpp"
#include <SDL3/SDL_gpu.h>

namespace {
	std::atomic_uint16_t g_MaterialIndex = 0;

	uint16 GenerateMaterialKey()
	{
		return g_MaterialIndex++ & 0x7FFF;
	}
} // namespace

namespace apollo::rdr {
	Material::Material(const ULID& id)
		: IAsset(id)
		, m_MaterialKey(GenerateMaterialKey())
	{}

	Material::~Material()
	{
		if (m_Handle)
		{
			SDL_ReleaseGPUGraphicsPipeline(
				Context::GetInstance()->GetDevice().GetHandle(),
				static_cast<SDL_GPUGraphicsPipeline*>(m_Handle));
		}
	}

	void MaterialInstance::Reset()
	{
		m_State = EAssetState::Invalid;
		m_Material.Reset();
		rdr::Context* ctx = rdr::Context::GetInstance();
		auto* device = ctx->GetDevice().GetHandle();

		for (uint32 i = 0; i < m_VertexTextures.m_NumTextures; ++i)
		{
			m_VertexTextures.m_Textures[i].Reset();
			if (m_VertexTextures.m_Samplers[i])
				SDL_ReleaseGPUSampler(device, m_VertexTextures.m_Samplers[i]);
		}
		for (uint32 i = 0; i < m_FragmentTextures.m_NumTextures; ++i)
		{
			m_FragmentTextures.m_Textures[i].Reset();
			if (m_FragmentTextures.m_Samplers[i])
				SDL_ReleaseGPUSampler(device, m_FragmentTextures.m_Samplers[i]);
		}
		m_VertexTextures.m_NumTextures = 0;
		m_FragmentTextures.m_NumTextures = 0;
	}

	MaterialInstance::~MaterialInstance()
	{
		Reset();
	}

	void MaterialInstance::PushFragmentConstants(SDL_GPUCommandBuffer* cmdBuffer, uint32 index) const
	{
		APOLLO_ASSERT(index < 4, "Block index {} is out of bounds", index);
		SDL_PushGPUFragmentUniformData(
			cmdBuffer,
			index,
			m_ConstantBlocks.m_FragmentConstants[index].m_Buffer,
			m_ConstantBlocks.m_BlockSizes[index]);
	}

	void MaterialInstance::Bind(SDL_GPURenderPass* renderPass) const
	{
		if (!IsLoaded())
			return;
		auto* context = Context::GetInstance();

		SDL_BindGPUGraphicsPipeline(
			renderPass,
			static_cast<SDL_GPUGraphicsPipeline*>(m_Material->GetHandle()));

		SDL_GPUTextureSamplerBinding bindings[s_MaxTextures] = {};
		for (uint32 i = 0; i < m_VertexTextures.m_NumTextures; ++i)
		{
			bindings[i].texture = m_VertexTextures.m_Textures[i]->GetHandle();
			bindings[i].sampler = m_VertexTextures.m_Samplers[i] ? m_VertexTextures.m_Samplers[i]
																 : context->GetDefaultSampler();
		}
		SDL_BindGPUVertexSamplers(renderPass, 0, bindings, m_VertexTextures.m_NumTextures);

		for (uint32 i = 0; i < m_FragmentTextures.m_NumTextures; ++i)
		{
			bindings[i].texture = m_FragmentTextures.m_Textures[i]->GetHandle();
			bindings[i].sampler = m_FragmentTextures.m_Samplers[i]
									  ? m_FragmentTextures.m_Samplers[i]
									  : context->GetDefaultSampler();
		}
		SDL_BindGPUFragmentSamplers(renderPass, 0, bindings, m_FragmentTextures.m_NumTextures);
	}
} // namespace apollo::rdr