import "Apollo.Core";

static const float2 g_Offset[] = 
{
	float2(0, 0),
	float2(0, 1),
	float2(1, 0),
	float2(1, 1),
};

cbuffer CameraData: register(b0, space1)
{
	float4x4 VPMatrix;
}

struct Border
{
	float4 Bounds;
	float4 Color;
	float4 Width;
	float4 CornerRadius;
};

struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 RelativePos;
	float2 HalfSize;
	float4 Width;
	float4 CornerRadius;
	float4 Color;
};

StructuredBuffer<Border> g_Elems: register(t0, space0);

[[shader("vertex")]]
Fragment vs_main(uint id: SV_VertexID, uint instance: SV_InstanceID)
{
	Fragment f;
	f.HalfSize = 0.5 * g_Elems[instance].Bounds.zw;
	float2 offset = g_Elems[instance].Bounds.zw * g_Offset[id];
	
	f.Pos = mul(VPMatrix, float4(g_Elems[instance].Bounds.xy + offset, 0, 1));
	f.RelativePos = offset - f.HalfSize;
	f.Width = g_Elems[instance].Width;
	f.CornerRadius = g_Elems[instance].CornerRadius;
	f.Color = g_Elems[instance].Color;
	
	return f;
}

[[shader("fragment")]]
float4 fs_main(Fragment frag): SV_TARGET
{
	uint quadrant = (frag.RelativePos.x > 0) + ((frag.RelativePos.y > 0) << 1);
	float radius = frag.CornerRadius[quadrant];
	float d = apollo.BoxSdf(frag.RelativePos, frag.HalfSize, radius);
	float dw = 1.5 * fwidth(d);
	float outer = 1 - smoothstep(-dw, 0.0, d);
	
	float2 pos = frag.RelativePos - 0.5 * float2(frag.Width.x - frag.Width.y, frag.Width.z - frag.Width.w);
	radius -= max(
		frag.Width[uint(pos.x > 0)],
		frag.Width[(2 | uint(pos.y > 0))]
	);
	float2 halfSize = frag.HalfSize - 0.5 * float2(frag.Width.x + frag.Width.y, frag.Width.z + frag.Width.w);
	d = apollo.BoxSdf(pos, halfSize, max(radius, 0.0));
	dw = 1.5 * fwidth(d);
	float inner = 1 - smoothstep(-dw, 0.0, d);

	float alpha = outer - inner;
	return float4(frag.Color.rgb, frag.Color.a * alpha);
}