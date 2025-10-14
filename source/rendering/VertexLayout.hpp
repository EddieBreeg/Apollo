#pragma once

#include <PCH.hpp>

namespace apollo::rdr {
	struct VertexAttribute
	{
		enum EType : int8
		{
			Invalid = -1,
			Float,
			Float2,
			Float3,
			Float4,
			Int,
			Int2,
			Int3,
			Int4,
			UInt,
			UInt2,
			UInt3,
			UInt4,
			NTypes
		};
		EType m_Type;
		uint32 m_Offset;
	};

	template <class T>
	consteval VertexAttribute::EType GetAttributeType() noexcept;

	template <auto... Members>
	requires(std::is_member_pointer_v<decltype(Members)>&&...) struct VertexAttributeList
	{
		static constexpr uint32 Size = sizeof...(Members);
		VertexAttribute Attributes[Size];

		consteval VertexAttributeList() noexcept
		{
			uint32 index = 0;
			uint32 offset = 0;
			((offset = GetAttribute(Attributes[index++], offset, Members)), ...);
		}

		constexpr const VertexAttribute& operator[](uint32 i) const { return Attributes[i]; }

	private:
		template <class V, class M>
		static consteval uint32
		GetAttribute(VertexAttribute& out_attr, uint32 offset, M V::*) noexcept
		{
			offset = Align(offset, alignof(M));
			out_attr.m_Type = GetAttributeType<M>();
			out_attr.m_Offset = offset;
			return offset + sizeof(M);
		}
	};

#define DECL_VERTEX_ATTRIBUTES(...) static constexpr VertexAttributeList<__VA_ARGS__> Attributes

	namespace _internal {
		template <class T>
		struct IsVertexAttributeList : std::false_type
		{};

		template <auto... Ptr>
		struct IsVertexAttributeList<VertexAttributeList<Ptr...>> : std::true_type
		{};

		template <class T>
		struct IsVertexAttributeList<const T> : IsVertexAttributeList<T>
		{};
	} // namespace _internal

	template <>
	consteval VertexAttribute::EType GetAttributeType<float>() noexcept
	{
		return VertexAttribute::Float;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<float2>() noexcept
	{
		return VertexAttribute::Float2;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<float3>() noexcept
	{
		return VertexAttribute::Float3;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<float4>() noexcept
	{
		return VertexAttribute::Float4;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<int32>() noexcept
	{
		return VertexAttribute::Int;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::ivec2>() noexcept
	{
		return VertexAttribute::Int2;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::ivec3>() noexcept
	{
		return VertexAttribute::Int3;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::ivec4>() noexcept
	{
		return VertexAttribute::Int4;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<uint32>() noexcept
	{
		return VertexAttribute::UInt;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::uvec2>() noexcept
	{
		return VertexAttribute::UInt2;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::uvec3>() noexcept
	{
		return VertexAttribute::UInt3;
	}
	template <>
	consteval VertexAttribute::EType GetAttributeType<glm::uvec4>() noexcept
	{
		return VertexAttribute::UInt4;
	}
} // namespace apollo::rdr