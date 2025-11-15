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

	void MaterialInstance::ConstantStorage::Init(std::span<const ShaderConstantBlock> blocks)
	{
		APOLLO_ASSERT(blocks.size() <= 4, "Too many shader constant blocks");
		uint32 totalSize = 0;

		for (size_t i = 0; i < blocks.size(); ++i)
		{
			const uint32 s = m_Sizes[i] = blocks[i].m_Size;
			m_Offsets[i] = totalSize;
			totalSize += s;
		}
		m_Ptr = std::make_unique<uint8[]>(totalSize);
	}

	uint8* MaterialInstance::ConstantStorage::GetBlockStart(uint32 index)
	{
		APOLLO_ASSERT(
			index < 4 && m_Sizes[index],
			"Constant block index {} is out out bounds",
			index);
		return m_Ptr.get() + m_Offsets[index];
	}
	const uint8* MaterialInstance::ConstantStorage::GetBlockStart(uint32 index) const
	{
		APOLLO_ASSERT(
			index < 4 && m_Sizes[index],
			"Constant block index {} is out out bounds",
			index);
		return m_Ptr.get() + m_Offsets[index];
	}

	uint8* MaterialInstance::ConstantStorage::GetConstantPtr(
		uint32 block,
		uint32 offset,
		uint32 size)
	{
		uint8* blockStart = GetBlockStart(block);
		APOLLO_ASSERT(
			(offset + size) <= m_Sizes[block],
			"Offset {} is out of bounds for block [{}]: block size is {} and variable size is {}",
			offset,
			block,
			m_Sizes[block],
			size);
		return blockStart + offset;
	}
	const uint8* MaterialInstance::ConstantStorage::GetConstantPtr(
		uint32 block,
		uint32 offset,
		uint32 size) const
	{
		const uint8* blockStart = GetBlockStart(block);
		APOLLO_ASSERT(
			(offset + size) <= m_Sizes[block],
			"Offset {} is out of bounds for block [{}]: block size is {} and variable size is {}",
			offset,
			block,
			m_Sizes[block],
			size);
		return blockStart + offset;
	}

	void MaterialInstance::PushFragmentConstants(SDL_GPUCommandBuffer* cmdBuffer, uint32 index) const
	{
		const uint8* ptr = m_ConstantBlocks.GetBlockStart(index);
		SDL_PushGPUFragmentUniformData(cmdBuffer, index, ptr, m_ConstantBlocks.m_Sizes[index]);
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