#include "VertexTypes.hpp"
#include <SDL3/SDL_gpu.h>
#include <array>
#include <core/Assert.hpp>

namespace {
	constexpr SDL_GPUVertexElementFormat g_ElementFormats[] = {
		SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,	SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
		SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
		SDL_GPU_VERTEXELEMENTFORMAT_INT,	SDL_GPU_VERTEXELEMENTFORMAT_INT2,
		SDL_GPU_VERTEXELEMENTFORMAT_INT3,	SDL_GPU_VERTEXELEMENTFORMAT_INT4,
		SDL_GPU_VERTEXELEMENTFORMAT_UINT,	SDL_GPU_VERTEXELEMENTFORMAT_UINT2,
		SDL_GPU_VERTEXELEMENTFORMAT_UINT3,	SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
	};

	static_assert(STATIC_ARRAY_SIZE(g_ElementFormats) == brk::rdr::VertexAttribute::NTypes);

	template <brk::rdr::StandardVertexType V>
	consteval auto StandardAttr()
	{
		std::array<SDL_GPUVertexAttribute, V::Attributes.Size> arr = {};

		for (uint32 i = 0; i < arr.size(); ++i)
		{
			arr[i] = SDL_GPUVertexAttribute{
				.location = i,
				.buffer_slot = 0,
				.format = g_ElementFormats[int8(V::Attributes[i].m_Type)],
				.offset = V::Attributes[i].m_Offset,
			};
		}
		return arr;
	}

	constexpr auto g_Vertex2dAttributes = StandardAttr<brk::rdr::Vertex2d>();
	constexpr auto g_Vertex3dAttributes = StandardAttr<brk::rdr::Vertex3d>();

	constexpr std::span<const SDL_GPUVertexAttribute> g_StandardAttributes[] = {
		{ g_Vertex2dAttributes },
		{ g_Vertex3dAttributes },
	};
	static_assert(
		STATIC_ARRAY_SIZE(g_StandardAttributes) == (uint32)brk::rdr::EStandardVertexType::NTypes);

	constexpr SDL_GPUVertexBufferDescription g_StandardBufferDesc[] = {
		SDL_GPUVertexBufferDescription{
			.slot = 0,
			.pitch = sizeof(brk::rdr::Vertex2d),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		},
		SDL_GPUVertexBufferDescription{
			.slot = 0,
			.pitch = sizeof(brk::rdr::Vertex3d),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		}
	};

	static_assert(
		STATIC_ARRAY_SIZE(g_StandardBufferDesc) == (uint32)brk::rdr::EStandardVertexType::NTypes);

	constexpr SDL_GPUVertexInputState g_StandardInputStates[] = {
		SDL_GPUVertexInputState{
			.vertex_buffer_descriptions = g_StandardBufferDesc,
			.num_vertex_buffers = 1,
			.vertex_attributes = g_Vertex2dAttributes.data(),
			.num_vertex_attributes = (uint32)g_Vertex2dAttributes.size(),
		},
		SDL_GPUVertexInputState{
			.vertex_buffer_descriptions = g_StandardBufferDesc + 1,
			.num_vertex_buffers = 1,
			.vertex_attributes = g_Vertex3dAttributes.data(),
			.num_vertex_attributes = (uint32)g_Vertex3dAttributes.size(),
		},
	};

	static_assert(
		STATIC_ARRAY_SIZE(g_StandardInputStates) == (uint32)brk::rdr::EStandardVertexType::NTypes);
} // namespace

namespace brk::rdr {
	std::span<const SDL_GPUVertexAttribute> GetStandardVertexAttributes(EStandardVertexType type)
	{
		BRK_ASSERT(
			type > EStandardVertexType::Invalid && type < EStandardVertexType::NTypes,
			"Invalid standard vertex type {}",
			int32(type));
		return g_StandardAttributes[(int8)type];
	}

	const SDL_GPUVertexInputState& GetStandardVertexInputState(EStandardVertexType type)
	{
		BRK_ASSERT(
			type > EStandardVertexType::Invalid && type < EStandardVertexType::NTypes,
			"Invalid standard vertex type {}",
			int32(type));

		return g_StandardInputStates[int8(type)];
	}
} // namespace brk::rdr