struct Vertex
{
	float2 m_Position: POSITION;
	float2 m_Uv: TEXCOORD;
};

struct Fragment
{
	float4 Position: SV_POSITION;
	float2 Uv: TEXCOORD;
};

cbuffer CamData: register(b0, space1)
{
	float4x4 ViewProjMatrix;
};

Fragment main(Vertex v)
{
	Fragment res;
	res.Position = mul(ViewProjMatrix, float4(v.m_Position, 0, 1));
	res.Uv = v.m_Uv;
	return res;
}