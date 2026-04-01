struct Fragment
{
	float4 Pos: SV_POSITION;
	float2 RelativePos;
	float2 HalfSize;
	float4 Width;
	float4 CornerRadius;
	float4 Color;
};

float Sdf(float2 pos, float2 halfSize, float cornerRadius)
{
	pos = abs(pos) - halfSize + cornerRadius;
	return length(max(pos, 0.0)) + min(max(pos.x, pos.y), 0.0) - cornerRadius;
}

float4 main(Fragment frag): SV_TARGET
{
	uint quadrant = (frag.RelativePos.x > 0) + ((frag.RelativePos.y > 0) << 1);
	float radius = frag.CornerRadius[quadrant];
	float d = Sdf(frag.RelativePos, frag.HalfSize, radius);
	float dw = 1.5 * fwidth(d);
	float outer = 1 - smoothstep(-dw, 0.0, d);
	
	float2 pos = frag.RelativePos - 0.5 * float2(frag.Width.x - frag.Width.y, frag.Width.z - frag.Width.w);
	radius -= max(
		frag.Width[uint(pos.x > 0)],
		frag.Width[(2 | uint(pos.y > 0))]
	);
	float2 halfSize = frag.HalfSize - 0.5 * float2(frag.Width.x + frag.Width.y, frag.Width.z + frag.Width.w);
	d = Sdf(pos, halfSize, max(radius, 0.0));
	dw = 1.5 * fwidth(d);
	float inner = 1 - smoothstep(-dw, 0.0, d);

	float alpha = outer - inner;
	return float4(frag.Color.rgb, frag.Color.a * alpha);
}