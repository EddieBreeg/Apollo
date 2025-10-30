struct Fragment
{
	float4 Position: SV_POSITION;
	float3 UV: TEXCOORD;
};


cbuffer Params: register(b0, space3)
{
	float4 Color;
	uint Scale;
};

float4 main(Fragment frag): SV_TARGET
{
	int3 pos = floor(frag.UV * Scale);
	float fac = float((pos.x + pos.y + pos.z) % 2);
	return fac * Color;
}