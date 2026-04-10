struct Fragment
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD;
};

struct Textures {
	Sampler2D Color;
};
[[vk::binding(0, 2)]]
ParameterBlock<Textures> g_Tex;

cbuffer Params: register(b0, space3)
{
	float4 Color;
	float AlphaThreshold;
};

[[shader("fragment")]]
float4 main(Fragment frag): SV_TARGET
{
	float4 c = g_Tex.Color.Sample(frag.UV);
	if(c.r < AlphaThreshold)
		discard;

	return c * Color;
}