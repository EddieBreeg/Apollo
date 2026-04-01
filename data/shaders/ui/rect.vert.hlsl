static const float2 g_Offset[] = {
	float2(0, 0),
	float2(0, 1),
	float2(1, 0),
	float2(1, 1),
};

cbuffer Camera: register(b0, space1)
{
	float4x4 VPMatrix;
}

struct Element
{
	float4 Bounds;
	float4 BgColor;
	float4 CornerRadius;
};

StructuredBuffer<Element> g_Elems: register(t0, space0);

struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 RelativePos;
	Element Elem;
};

Fragment main(uint id: SV_VulkanVertexID, uint instance: SV_VulkanInstanceID)
{
	float2 offset = g_Elems[instance].Bounds.zw * g_Offset[id];
	float2 halfSize = 0.5 * g_Elems[instance].Bounds.zw;
	
	Fragment f;
	f.Pos = mul(VPMatrix, float4(g_Elems[instance].Bounds.xy + offset, 0, 1));
	f.RelativePos = offset - halfSize;
	f.Elem = g_Elems[instance];
	
	return f;
}