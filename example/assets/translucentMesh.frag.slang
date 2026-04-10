struct Fragment
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD;
};

[[vk::binding(0, 2)]]
Sampler2D g_Color;

cbuffer Params: register(b0, space3)
{
	float4 Color;
};

[[shader("fragment")]]
float4 main(Fragment frag): SV_TARGET
{
	float4 c = g_Color.Sample(frag.UV);
	return c * Color;
}