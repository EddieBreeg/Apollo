struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 Uv: TEXCOORD;
	float4 Color;
	float4 OutlineColor;
	float OutlineThickness;
};

Texture2D g_Tex: register(t0, space2);
SamplerState g_Sampler: register(s0, space2);

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
	float outerThreshold = 0.5 - frag.OutlineThickness;

	float outline = Outline(d, w, innerThreshold, outerThreshold);
	float fill = smoothstep(innerThreshold - w, innerThreshold + w, d);

	return fill * frag.Color + outline * frag.OutlineColor;
}