[[vk::binding(0, 2)]]
Sampler2D g_Tex[4];

[[vk::binding(0, 3)]]
ConstantBuffer<float4x4> g_Constants[2];

[[shader("fragment")]]
float4 fs_main(float4 pos: SV_Position, float2 uv: TEXCOORD)
{
	return g_Tex[0].Sample(uv);
}