struct Fragment {
	float4 Pos: SV_POSITION;
	float2 RelativePos;
};

cbuffer UiRect: register(b0, space3)
{
	float4 Rectangle;
	float4 BgColor;
	float4 BorderColor;
	float4 BorderThickness;
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
	float corner = CornerRadius[quadrant];

	float2 halfSize = 0.5 * Rectangle.zw;
	float d = Sdf(frag.RelativePos, halfSize, corner);
	float dw = fwidth(d);
	float alpha = smoothstep(d - dw, d, 0.0);

	halfSize -= 0.5 * float2(BorderThickness.x + BorderThickness.z, BorderThickness.y + BorderThickness.w);
	float2 offset = float2(BorderThickness.x - BorderThickness.z, BorderThickness.y - BorderThickness.w);
	d = Sdf(frag.RelativePos + offset, halfSize, corner);
	float inner = smoothstep(d - dw, d, 0.0);

	float4 color = lerp(BorderColor, BgColor, inner);

	return float4(color.rgb, color.a * alpha);
}