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
};

float Median(float a, float b, float c)
{
	return max(min(a, b), min(max(a, b), c));
}

float Chebichev(float2 v)
{
	return max(abs(v.x), abs(v.y));
}

float Outline(float d, float w, float inner, float outer)
{
	return smoothstep(outer - w, outer + w, d) - smoothstep(inner - w, inner + w, d);
}

float4 main(Fragment frag): SV_TARGET
{
	float3 px = g_Tex.Sample(g_Sampler, frag.Uv).xyz;
	float d = Median(px.x, px.y, px.z);
	float w = fwidth(d);

	float innerThreshold = 0.5;
	float outerThreshold = 0.5 - OutlineWidth;

	float outline = Outline(d, w, innerThreshold, outerThreshold);
	float fill = smoothstep(innerThreshold - w, innerThreshold + w, d);

	float d2 = Chebichev(frag.Uv - 0.5);
	float borderFac = smoothstep(0.5 - 2* fwidth(d2), 0.5, d2);

	float fac = outline + borderFac;

	return fill * float4(1) + fac * float4(1, 0, 0, 1);
}