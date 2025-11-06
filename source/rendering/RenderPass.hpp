#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPURenderPass;

namespace apollo::rdr {
	class Context;
	class Texture2D;

	enum class ELoadOp : int8
	{
		Invalid = -1,
		Clear,
		Load,
		DontCare,
		NOperations
	};
	enum class EStoreOp : int8
	{
		Invalid = -1,
		Store,
		Resolve,
		ResolveAndStore,
		DontCare,
		NOperations
	};

	struct ColorTargetSettings
	{
		const Texture2D* m_Texture = nullptr;
		ELoadOp m_LoadOp = ELoadOp::Clear;
		EStoreOp m_StoreOp = EStoreOp::Store;
		float4 m_ClearColor = {};
	};

	struct DepthStencilTargetSettings
	{
		ELoadOp m_LoadOp = ELoadOp::Clear;
		EStoreOp m_StoreOp = EStoreOp::Store;
		ELoadOp m_StencilLoadOp = ELoadOp::Clear;
		EStoreOp m_StencilStoreOp = EStoreOp::Store;

		const Texture2D* m_Texture = nullptr;
		float m_ClearDepth = 1.0f;
	};

	struct RenderPassSettings
	{
		uint8 m_NumColorTargets = 1;
		ColorTargetSettings m_ColorTargets[8] = {};
		DepthStencilTargetSettings m_DepthStencilTarget;
	};

	class RenderPass : public _internal::HandleWrapper<SDL_GPURenderPass*>
	{
	public:
		APOLLO_API RenderPass(const RenderPassSettings& settings);
		APOLLO_API void Begin(Context& cmdBuffer);
		APOLLO_API void End();

		APOLLO_API ~RenderPass();

	private:
		RenderPassSettings m_Settings;
	};
} // namespace apollo::rdr