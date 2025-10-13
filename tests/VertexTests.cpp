#include <SDL3/SDL_gpu.h>
#include <catch2/catch_test_macros.hpp>
#include <rendering/VertexTypes.hpp>

#define VERTEX_TEST(name) TEST_CASE(name, "[vertex][rdr]")

namespace brk::rdr::vertex_ut {
	struct V1
	{
		float x;
		int32 y;
		uint32 z;

		float2 f2;
		glm::ivec2 i2;
		glm::uvec2 u2;

		float3 f3;
		glm::ivec3 i3;
		glm::uvec3 u3;

		float4 f4;
		glm::ivec4 i4;
		glm::uvec4 u4;

		DECL_VERTEX_ATTRIBUTES(
			&V1::x,
			&V1::y,
			&V1::z,
			&V1::f2,
			&V1::i2,
			&V1::u2,
			&V1::f3,
			&V1::i3,
			&V1::u3,
			&V1::f4,
			&V1::i4,
			&V1::u4);
	};

#define TEST_ATTR(index, name, type)                                                               \
	static_assert(V1::Attributes[index].m_Offset == offsetof(V1, name));                           \
	static_assert(V1::Attributes[index].m_Type == (type))

	VERTEX_TEST("V1 layout test")
	{
		static_assert(VertexType<V1>);
		static_assert(!StandardVertexType<V1>);
		static_assert(V1::Attributes.Size == 12);

		TEST_ATTR(0, x, VertexAttribute::Float);
		TEST_ATTR(1, y, VertexAttribute::Int);
		TEST_ATTR(2, z, VertexAttribute::UInt);

		TEST_ATTR(3, f2, VertexAttribute::Float2);
		TEST_ATTR(4, i2, VertexAttribute::Int2);
		TEST_ATTR(5, u2, VertexAttribute::UInt2);

		TEST_ATTR(6, f3, VertexAttribute::Float3);
		TEST_ATTR(7, i3, VertexAttribute::Int3);
		TEST_ATTR(8, u3, VertexAttribute::UInt3);

		TEST_ATTR(9, f4, VertexAttribute::Float4);
		TEST_ATTR(10, i4, VertexAttribute::Int4);
		TEST_ATTR(11, u4, VertexAttribute::UInt4);
	}

	VERTEX_TEST("Vertex2d Layout test")
	{
		const std::span attr = GetStandardVertexAttributes(EStandardVertexType::Vertex2d);
		CHECK(attr.size() == Vertex2d::Attributes.Size);

		CHECK(attr[0].location == 0);
		CHECK(attr[0].buffer_slot == 0);
		CHECK(attr[0].offset == 0);
		CHECK(attr[0].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2);

		CHECK(attr[1].location == 1);
		CHECK(attr[1].buffer_slot == 0);
		CHECK(attr[1].offset == offsetof(Vertex2d, m_Uv));
		CHECK(attr[1].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2);
	}

	VERTEX_TEST("Vertex3d Layout test")
	{
		const std::span attr = GetStandardVertexAttributes(EStandardVertexType::Vertex3d);
		CHECK(attr.size() == Vertex3d::Attributes.Size);

		CHECK(attr[0].location == 0);
		CHECK(attr[0].buffer_slot == 0);
		CHECK(attr[0].offset == 0);
		CHECK(attr[0].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3);

		CHECK(attr[1].location == 1);
		CHECK(attr[1].buffer_slot == 0);
		CHECK(attr[1].offset == offsetof(Vertex3d, m_Normal));
		CHECK(attr[1].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3);

		CHECK(attr[2].location == 2);
		CHECK(attr[2].buffer_slot == 0);
		CHECK(attr[2].offset == offsetof(Vertex3d, m_Uv));
		CHECK(attr[2].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2);
	}

#undef TEST_ATTR
} // namespace brk::rdr::vertex_ut