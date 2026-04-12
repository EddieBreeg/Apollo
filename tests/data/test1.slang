[[vk::binding(1, 0)]]
StructuredBuffer<float4x4> g_Transform;

[[vk::binding(0, 1)]]
ConstantBuffer<float4x4> g_Camera;

struct Fragment {
	float4 Pos: SV_Position;
	float2 Uv: TexCoord;
};

[[shader("vertex")]]
Fragment vs_main(float3 pos: Position, float2 uv, uint instance: SV_InstanceID)
{
	return Fragment(
		mul(g_Camera, mul(g_Transform[instance],float4(pos, 1))),
	uv);
}

struct Textures {
	Sampler2D Tex1;
	SamplerState Sampler;
};

[[vk::binding(0, 2)]]
ParameterBlock<Textures> g_Textures;

[[shader("fragment")]]
float4 fs_main(Fragment frag)
{
	return g_Textures.Tex1.Sample(frag.Uv);
}