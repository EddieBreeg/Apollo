struct Vertex3d
{
	float3 Position: POSITION;
	float3 Normal: NORMAL;
	float2 UV: TEXCOORD;
};

struct Fragment
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD;
};

cbuffer Camera: register(b0, space1)
{
	float4x4 VPMatrix;
};
cbuffer Transform: register(b1, space1)
{
	float4x4 ModelMatrix;
};

Fragment main(Vertex3d v)
{
	Fragment frag;
	frag.Position = mul(VPMatrix, mul(ModelMatrix, float4(v.Position.xyz, 1)));
	frag.UV = v.UV;
	return frag;
}