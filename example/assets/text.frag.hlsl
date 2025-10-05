struct Fragment
{
	float4 Position: SV_POSITION;
	float2 Uv: TEXCOORD;
};


Texture2D g_Tex: register(t0, space2);
SamplerState g_Sampler: register(s0, space2);

cbuffer Params: register(b0, space3)
{
	float OutlineWidth;
	float GlowIntensity;
	float GlowFalloff;
};

float Median(float a, float b, float c)
{
	return max(min(a, b), min(max(a, b), c));
}

float Outline(float d, float w, float inner, float outer)
{
	return smoothstep(outer - w, outer + w, d) - smoothstep(inner - w, inner + w, d);
}

float4 main(Fragment frag): SV_TARGET
{
	float4 px = g_Tex.Sample(g_Sampler, frag.Uv);
	float d = Median(px.r, px.g, px.b);
	float w = fwidth(d);

	float innerThreshold = 0.5;
	float outerThreshold = 0.5 - OutlineWidth;

	float outline = Outline(d, w, innerThreshold, outerThreshold);
	float fill = smoothstep(innerThreshold - w, innerThreshold + w, d);

	float glow = GlowIntensity * pow(px.a, 4 - GlowFalloff);

	return fill * float4(1) + outline * float4(1, 0, 0, 1) + glow;
}