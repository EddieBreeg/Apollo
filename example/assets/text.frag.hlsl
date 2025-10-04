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

float Cheb(float2 p)
{
	return max(abs(p.x - 0.5), abs(p.y - 0.5));
}


float4 main(Fragment frag): SV_TARGET
{
	float3 px = g_Tex.Sample(g_Sampler, frag.Uv).xyz;
	float d = Median(px.x, px.y, px.z);
	float w = fwidth(d);

	float outer = smoothstep(0.5 - OutlineWidth - w, 0.5 - OutlineWidth + w, d);
	float inner = smoothstep(0.5 - w, 0.5 + w, d);

	float d2 = Cheb(frag.Uv);
	float borderFac = smoothstep(0.5 - 2* fwidth(d2), 0.5, d2);

	return inner * float4(1) + (outer - inner + borderFac) * float4(1, 0, 0, 1);
}