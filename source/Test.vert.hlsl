
float4 vs_main(float3 pos: POSITION): SV_POSITION
{
	return float4(pos, 1);
}