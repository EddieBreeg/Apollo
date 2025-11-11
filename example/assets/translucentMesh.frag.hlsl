struct Fragment
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD;
};

Texture2D g_Color: register(t0, space2);
SamplerState g_Sampler0: register(s0, space2);

cbuffer Params: register(b0, space3)
{
	float4 Color;
};

float4 main(Fragment frag): SV_TARGET
{
	float4 c = g_Color.Sample(g_Sampler0, frag.UV);
	return c * Color;
}