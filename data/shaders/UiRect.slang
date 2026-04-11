import "Apollo.Core";

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

[[shader("vertex")]]
Fragment vs_main(uint id: SV_VertexID, uint instance: SV_InstanceID)
{
	float2 offset = g_Elems[instance].Bounds.zw * g_Offset[id];
	float2 halfSize = 0.5 * g_Elems[instance].Bounds.zw;
	
	Fragment f;
	f.Pos = mul(VPMatrix, float4(g_Elems[instance].Bounds.xy + offset, 0, 1));
	f.RelativePos = offset - halfSize;
	f.Elem = g_Elems[instance];
	
	return f;
}

[[shader("fragment")]]
float4 fs_main(Fragment frag): SV_TARGET
{
	uint quadrant = uint(frag.RelativePos.x > 0.0) + (uint(frag.RelativePos.y > 0) << 1);
	float corner = frag.Elem.CornerRadius[quadrant];

	float2 halfSize = 0.5 * frag.Elem.Bounds.zw;
	float d = apollo.BoxSdf(frag.RelativePos, halfSize, corner);
	float dw = 1.5 * fwidth(d);
	float alpha = 1.0 - smoothstep(-dw, 0.0, d);

	return float4(frag.Elem.BgColor.rgb, frag.Elem.BgColor.a * alpha);
}