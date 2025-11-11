#pragma once

#include <asset/AssetRef.hpp>
#include <ecs/Reflection.hpp>
#include <rendering/Material.hpp>
#include <rendering/Mesh.hpp>

namespace apollo::demo {
	struct MeshComponent
	{
		AssetRef<rdr::Mesh> m_Mesh;
		AssetRef<rdr::MaterialInstance> m_Material;

		static constexpr ecs::ComponentReflection<&MeshComponent::m_Mesh, &MeshComponent::m_Material>
			Reflection{
				"mesh",
				{ "mesh", "material" },
			};
	};
} // namespace apollo::demo