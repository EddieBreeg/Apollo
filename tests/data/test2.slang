struct Transform {
	float4x4 Base;
	float4 Offset;
};

struct Params{
	ConstantBuffer<float4x4> Camera;
	ConstantBuffer<Transform> Transform;
};
[[vk::binding(0, 1)]]
ParameterBlock<Params> g_Params;

[[shader("vertex")]]
float4 vs_main(float3 pos: Position): SV_Position
{
	return mul(
		g_Params.Camera,
		mul(
			g_Params.Transform.Base,
			float4(pos, 1)
		) + g_Params.Transform.Offset
	);
}