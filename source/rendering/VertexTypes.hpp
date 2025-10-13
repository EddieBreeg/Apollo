#include "VertexLayout.hpp"
#include <span>

struct SDL_GPUVertexAttribute;
struct SDL_GPUVertexInputState;

namespace brk::rdr {
	enum class EStandardVertexType : int8
	{
		Invalid = -1,
		Vertex2d,
		Vertex3d,
		NTypes
	};

	template <class V>
	concept VertexType = _internal::IsVertexAttributeList<decltype(V::Attributes)>::value;

	struct Vertex2d
	{
		float2 m_Position;
		float2 m_Uv;

		DECL_VERTEX_ATTRIBUTES(&Vertex2d::m_Position, &Vertex2d::m_Uv);
		static constexpr EStandardVertexType Type = EStandardVertexType::Vertex2d;
	};

	struct Vertex3d
	{
		float3 m_Position;
		float3 m_Normal;
		float2 m_Uv;

		DECL_VERTEX_ATTRIBUTES(&Vertex3d::m_Position, &Vertex3d::m_Normal, &Vertex3d::m_Uv);
		static constexpr EStandardVertexType Type = EStandardVertexType::Vertex3d;
	};

	template <class V>
	concept StandardVertexType = VertexType<V> &&
								 std::is_same_v<decltype(V::Type), const EStandardVertexType>;

	BRK_API std::span<const SDL_GPUVertexAttribute> GetStandardVertexAttributes(
		EStandardVertexType type);

	BRK_API const SDL_GPUVertexInputState& GetStandardVertexInputState(EStandardVertexType type);
} // namespace brk::rdr