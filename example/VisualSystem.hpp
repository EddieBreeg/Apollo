#pragma once

#include <PCH.hpp>

#include "CameraSystem.hpp"
#include "Inspector.hpp"

namespace apollo::demo {
	struct VisualElement
	{
		enum EType : uint8
		{
			Mesh,
			Grid,
		};

		VisualElement(
			const MeshComponent& meshComp,
			const TransformComponent& transform,
			float3 camPos,
			float3 viewVec);
		VisualElement(
			const GridComponent& gridComponent,
			const TransformComponent& transform,
			float3 camPos,
			float3 viewVec);

		void Draw(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const;
		[[nodiscard]] uint64 GetKey() const noexcept { return m_Key; }

	private:
		void DrawMesh(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const;
		void DrawGrid(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const;

		const TransformComponent* m_Transform;
		union {
			const MeshComponent* m_MeshComp;
			const GridComponent* m_Grid;
		};
		uint64 m_Key;
		EType m_Type;
	};

	[[nodiscard]] inline bool operator<(const VisualElement& lhs, const VisualElement& rhs) noexcept
	{
		return lhs.GetKey() < rhs.GetKey();
	}

	struct VisualSystem
	{
		VisualSystem(
			apollo::Window& window,
			apollo::rdr::Context& renderer,
			CameraSystem& camSystem);

		~VisualSystem() {}

		void Update(entt::registry& world, const apollo::GameTime&);
		const Viewport& GetTargetViewport() const noexcept { return m_TargetViewport; }

		Inspector m_Inspector;

	private:
		void DisplayUi(entt::registry& world);
		void EmitGPUCommands(const entt::registry& world);

		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		CameraSystem& m_CamSystem;
		Viewport m_TargetViewport;
		rdr::RenderPass m_RenderPass;

		std::vector<VisualElement> m_VisualElements;
	};

} // namespace apollo::demo