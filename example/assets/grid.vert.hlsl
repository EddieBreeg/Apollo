cbuffer FrameData: register(b0, space1)
{
	float4x4 VP;
};
cbuffer GridData: register(b1, space1)
{
	float4x4 Transform;
	uint GridWidth;
};

struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 Uv: TEXCOORD;
	uint Instance: INSTANCEID;
};

static const float3 g_Offsets[] = {
	float3(0, 0, 0),
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(1, 1, 0),
};
static const float2 g_Uv[] = {
	float2(0, 1),
	float2(1, 1),
	float2(0, 0),
	float2(1, 0),
};

Fragment main(uint vertex: SV_VulkanVertexID, uint instance: SV_VulkanInstanceID)
{
	Fragment frag;
	float scale = 2.0 / GridWidth;

	uint i = instance % GridWidth;
	uint j = instance / GridWidth;
	float4 pos = float4(scale * (float3(i, j, 0) + g_Offsets[vertex]) + float3(-1, -1, 0), 1);
	frag.Pos = mul(VP, mul(Transform, pos));
	frag.Uv = g_Uv[vertex];
	frag.Instance = instance;
	
	return frag;
}