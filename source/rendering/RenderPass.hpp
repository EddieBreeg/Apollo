#pragma once

/** \file RenderPass.hpp */

#include <PCH.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPURenderPass;

namespace apollo::rdr {
	class Context;
	class Texture2D;

	/// The operation to perform on a target at the start of a render pass
	enum class ELoadOp : int8
	{
		Invalid = -1,
		Clear,
		Load,
		DontCare,
		NOperations
	};
	/// The operation to perform on a target when storing a pixel value
	enum class EStoreOp : int8
	{
		Invalid = -1,
		Store,
		Resolve,
		ResolveAndStore,
		DontCare,
		NOperations
	};

	/**
	 * \brief Specifies render pass color target
	 * \sa RenderPassSettings
	 */
	struct ColorTargetSettings
	{
		const Texture2D* m_Texture = nullptr; /*!< If nullptr, the swapchain texture will be used */
		ELoadOp m_LoadOp = ELoadOp::Clear;
		EStoreOp m_StoreOp = EStoreOp::Store;
		float4 m_ClearColor = {};
	};

	/**
	 * \brief Specifies render pass depth stencil target
	 * \sa RenderPassSettings
	 */
	struct DepthStencilTargetSettings
	{
		ELoadOp m_LoadOp = ELoadOp::Clear;
		EStoreOp m_StoreOp = EStoreOp::Store;
		ELoadOp m_StencilLoadOp = ELoadOp::Clear;
		EStoreOp m_StencilStoreOp = EStoreOp::Store;

		const Texture2D* m_Texture = nullptr; /*!< If nullptr, the depth stencil target will be
												 ignored entirely */
		float m_ClearDepth = 1.0f;
	};

	/**
	 * \brief Specifies the targets of a render pass
	 * \sa RenderPass
	 */
	struct RenderPassSettings
	{
		uint8 m_NumColorTargets = 1;
		ColorTargetSettings m_ColorTargets[8] = {};
		DepthStencilTargetSettings m_DepthStencilTarget;
	};

	/**
	 * \brief Render pass abstraction. All GPU draw operations must be performed during a render
	 * pass.
	 */
	class RenderPass : public _internal::HandleWrapper<SDL_GPURenderPass*>
	{
	public:
		APOLLO_API RenderPass(const RenderPassSettings& settings);
		APOLLO_API void Begin(Context& ctx);
		APOLLO_API void End();

		APOLLO_API ~RenderPass();

	private:
		RenderPassSettings m_Settings;
	};
} // namespace apollo::rdr