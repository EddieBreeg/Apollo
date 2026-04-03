struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 Uv: TEXCOORD;
	uint Instance: INSTANCEID;
};
[[shader("fragment")]]
float4 main(Fragment frag): SV_TARGET
{
	return float4(frag.Uv, 1, 1);
}