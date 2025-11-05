#include "RenderPass.hpp"
#include "Context.hpp"
#include "Texture.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>

namespace {
	constexpr SDL_GPULoadOp g_LoadOps[] = {
		SDL_GPU_LOADOP_CLEAR,
		SDL_GPU_LOADOP_LOAD,
		SDL_GPU_LOADOP_DONT_CARE,
	};
	constexpr SDL_GPUStoreOp g_StoreOps[] = {
		SDL_GPU_STOREOP_STORE,
		SDL_GPU_STOREOP_RESOLVE,
		SDL_GPU_STOREOP_RESOLVE_AND_STORE,
		SDL_GPU_STOREOP_DONT_CARE,
	};

#ifdef APOLLO_DEV
#define ASSERT_OP_ENUM_VALID(OpType, val)                                                          \
	APOLLO_ASSERT(                                                                                 \
		OpType::Invalid < val && val < OpType::NOperations,                                        \
		"Invalid value for " #OpType ": {}",                                                       \
		static_cast<int8>(val))

	void ValidateSettings(const apollo::rdr::RenderPassSettings& settings)
	{
		using namespace apollo::rdr;
		ASSERT_OP_ENUM_VALID(ELoadOp, settings.m_DepthStencilTarget.m_LoadOp);
		ASSERT_OP_ENUM_VALID(EStoreOp, settings.m_DepthStencilTarget.m_StoreOp);

		uint8 numNullTextures = 0;

		for (uint8 i = 0; i < settings.m_NumColorTargets; ++i)
		{
			const auto& target = settings.m_ColorTargets[i];
			ASSERT_OP_ENUM_VALID(ELoadOp, target.m_LoadOp);
			ASSERT_OP_ENUM_VALID(EStoreOp, target.m_StoreOp);
			if (!target.m_Texture)
				++numNullTextures;
		}
		APOLLO_ASSERT(
			numNullTextures < 2,
			"Only one color target may be assigned to the swapchain texture, but {} null textures "
			"were passed",
			numNullTextures);
	}
#undef ASSERT_OP_ENUM_VALID
#endif
} // namespace

namespace apollo::rdr {
	RenderPass::RenderPass(const RenderPassSettings& settings)
		: m_Settings(settings)
	{
#ifdef APOLLO_DEV
		ValidateSettings(settings);
#endif
	}

	void RenderPass::Begin(Context& ctx)
	{
		SDL_GPUColorTargetInfo colorTargets[8];
		const uint8 nTargets = Max(uint8(1), m_Settings.m_NumColorTargets);

		const auto& dsSettings = m_Settings.m_DepthStencilTarget;

		SDL_GPUDepthStencilTargetInfo dsTarget{
			.clear_depth = dsSettings.m_ClearDepth,
			.load_op = g_LoadOps[ToUnderlying(dsSettings.m_LoadOp)],
			.store_op = g_StoreOps[ToUnderlying(dsSettings.m_StoreOp)],
			.stencil_load_op = g_LoadOps[ToUnderlying(dsSettings.m_StencilLoadOp)],
			.stencil_store_op = g_StoreOps[ToUnderlying(dsSettings.m_StencilStoreOp)],
		};
		if (dsSettings.m_Texture)
			dsTarget.texture = dsSettings.m_Texture->GetHandle();

		for (uint32 i = 0; i < nTargets; ++i)
		{
			const auto& target = m_Settings.m_ColorTargets[i];

			colorTargets[i] = SDL_GPUColorTargetInfo{
				.clear_color = {
						target.m_ClearColor.r,
						target.m_ClearColor.g,
						target.m_ClearColor.b,
						target.m_ClearColor.a,
					},
				.load_op = g_LoadOps[ToUnderlying(target.m_LoadOp)],
				.store_op = g_StoreOps[ToUnderlying(target.m_StoreOp)],
			};

			if (target.m_Texture)
			{
				colorTargets[i].texture = target.m_Texture->GetHandle();
				continue;
			}

			colorTargets[i].texture = ctx.GetSwapchainTexture();
			APOLLO_ASSERT(
				colorTargets[i].texture,
				"Swapchain texture is used as a color target, but is null. Did you (forget to) "
				"create a window?");
		}

		m_Handle = SDL_BeginGPURenderPass(
			ctx.GetMainCommandBuffer(),
			colorTargets,
			nTargets,
			&dsTarget);
	}

	void RenderPass::End()
	{
		if (m_Handle) [[likely]]
		{
			SDL_EndGPURenderPass(m_Handle);
			m_Handle = nullptr;
		}
	}

	RenderPass::~RenderPass()
	{
		End();
	}
} // namespace apollo::rdr