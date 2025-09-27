struct Fragment
{
	float4 Position: SV_POSITION;
	float2 Uv: TEXCOORD;
};

Texture2D g_Tex: register(t0, space2);
SamplerState g_Sampler: register(s0, space2);

float4 main(Fragment frag): SV_TARGET
{
	return g_Tex.Sample(g_Sampler, frag.Uv);
}