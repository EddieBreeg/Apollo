struct Quad
{
	float4 m_Rect;
	float4 m_Uv;
	float4 m_MainColor;
	float4 m_OutlineColor;
	float m_OutlineThickness;
};

StructuredBuffer<Quad> g_Quads: register(t0, space0);

cbuffer Transform: register(b0, space1)
{
	float4x4 g_ProjMatrix;
	float4x4 g_ModelMatrix;
};

struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 Uv: TEXCOORD;
	float4 Color;
	float4 OutlineColor;
	float OutlineThickness;
};

const float2 g_PosOffsets[] = {
	float2(0, 0),
	float2(1, 0),
	float2(0, 1),
	float2(1, 1),
};

const float2 g_UvOffsets[] = {
	float2(0, 1),
	float2(1, 1),
	float2(0, 0),
	float2(1, 0),
};

Fragment main(uint instance: SV_INSTANCEID, uint index: SV_VERTEXID)
{
	Fragment frag;
	float4 rect = g_Quads[instance].m_Rect;
	float4 uvQuad = g_Quads[instance].m_Uv;

	frag.Pos = mul(g_ProjMatrix, mul(g_ModelMatrix, float4(
		rect.xy + rect.zw * g_PosOffsets[index],
		0,
		1
	)));
	frag.Uv = uvQuad.xy + uvQuad.zw * g_UvOffsets[index];
	frag.Color = g_Quads[instance].m_MainColor;
	frag.OutlineColor = g_Quads[instance].m_OutlineColor;
	frag.OutlineThickness = g_Quads[instance].m_OutlineThickness;

	return frag;
}