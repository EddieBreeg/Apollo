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

Fragment main(uint id: SV_VERTEXID, uint instance: SV_INSTANCEID)
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