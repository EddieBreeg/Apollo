struct Fragment {
	float4 Pos: SV_POSITION;
	float2 RelativePos;
	float4 Rectangle;
	float4 BgColor;
	float4 CornerRadius;
};

float Sdf(float2 pos, float2 halfSize, float cornerRadius)
{
	pos = abs(pos) - halfSize + cornerRadius;
	return length(max(pos, 0.0)) + min(max(pos.x, pos.y), 0.0) - cornerRadius;
}

float4 main(Fragment frag): SV_TARGET
{
	uint quadrant = uint(frag.RelativePos.x > 0.0) + (uint(frag.RelativePos.y > 0) << 1);
	float corner = frag.CornerRadius[quadrant];

	float2 halfSize = 0.5 * frag.Rectangle.zw;
	float d = Sdf(frag.RelativePos, halfSize, corner);
	float dw = 1.5 * fwidth(d);
	float alpha = 1.0 - smoothstep(-dw, 0.0, d);

	return float4(frag.BgColor.rgb, frag.BgColor.a * alpha);
}