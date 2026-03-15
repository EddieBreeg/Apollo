
static const float2 g_Offset[] = {
	float2(0, 0),
	float2(1, 0),
	float2(0, 1),
	float2(1, 1),
};

cbuffer Camera: register(b0, space1)
{
	float4x4 VPMatrix;
}

cbuffer Object: register(b1, space1)
{
	float4 Rectangle;
}

struct Fragment {
	float4 Pos: SV_POSITION;
	float2 RelativePos;
};

Fragment main(uint id: SV_VERTEXID)
{
	float2 offset = Rectangle.zw * g_Offset[id];
	float2 halfSize = 0.5 * Rectangle.zw;
	
	Fragment f;
	f.Pos = mul(VPMatrix, float4(Rectangle.xy + offset, 0, 1));
	f.RelativePos = offset - halfSize;
	
	return f;
}