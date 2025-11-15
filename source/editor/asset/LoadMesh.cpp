#include "AssetHelper.hpp"
#include <asset/AssetLoader.hpp>
#include <asset/AssetManager.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <core/Log.hpp>
#include <core/NumConv.hpp>
#include <rendering/Mesh.hpp>
#include <rendering/VertexTypes.hpp>

namespace {
	constexpr auto g_PostProcessFlags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
										aiProcess_FixInfacingNormals | aiProcess_OptimizeMeshes |
										aiProcess_OptimizeGraph | aiProcess_FlipUVs;
}

namespace apollo::editor {
	template <>
	EAssetLoadResult AssetHelper<rdr::Mesh>::Load(IAsset& out_asset, const AssetMetadata& metadata)
	{
		Assimp::Importer importer;
		// I hate this, but Assimp takes in a C string, and stupid Windows uses wide strings so we
		// need the conversion
		const std::string pathStr = metadata.m_FilePath.string();
		const aiScene* scene = importer.ReadFile(pathStr.c_str(), g_PostProcessFlags);
		if (!scene)
		{
			APOLLO_LOG_ERROR("Failed to load mesh from: {}", importer.GetErrorString());
			return EAssetLoadResult::Failure;
		}

		rdr::Mesh& mesh = static_cast<rdr::Mesh&>(out_asset);

		const aiMesh* am = scene->mMeshes[0];

		std::vector<rdr::Vertex3d> vertices;
		vertices.reserve(am->mNumVertices);
		std::vector<uint32> indices;
		indices.reserve(am->mNumFaces * 3);

		for (uint32 i = 0; i < am->mNumVertices; ++i)
		{
			const float3 pos{ am->mVertices[i].x, am->mVertices[i].y, am->mVertices[i].z };
			const float3 nor{ am->mNormals[i].x, am->mNormals[i].y, am->mNormals[i].z };
			const float2 uv{ am->mTextureCoords[0][i].x, am->mTextureCoords[0][i].y };
			vertices.emplace_back(rdr::Vertex3d{ pos, nor, uv });
		}

		for (uint32 i = 0; i < am->mNumFaces; ++i)
		{
			const auto& face = am->mFaces[i];
			for (uint32 j = 0; j < face.mNumIndices; ++j)
			{
				indices.emplace_back(face.mIndices[j]);
			}
		}

		mesh.m_NumVertices = am->mNumVertices;
		const uint32 vertSize = am->mNumVertices * sizeof(rdr::Vertex3d);
		mesh.m_NumIndices = apollo::NumCast<uint32>(indices.size());
		const uint32 indSize = 4 * mesh.m_NumIndices;

		mesh.m_VBuffer = rdr::Buffer(rdr::EBufferFlags::Vertex, vertSize);
		mesh.m_IBuffer = rdr::Buffer(rdr::EBufferFlags::Index, 4 * (uint32)indices.size());
		auto* const copyPass = AssetLoader::GetCurrentCopyPass();

		mesh.m_VBuffer.UploadData(copyPass, vertices.data(), vertSize);
		mesh.m_IBuffer.UploadData(copyPass, indices.data(), indSize);

		return EAssetLoadResult::Success;
	}
} // namespace apollo::editor