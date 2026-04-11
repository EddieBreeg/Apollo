module "Apollo.Core";

namespace apollo {
	public float BoxSdf(float2 pos, float2 halfSize, float cornerRadius)
	{
		pos = abs(pos) - halfSize + cornerRadius;
		return length(max(pos, 0.0)) + min(max(pos.x, pos.y), 0.0) - cornerRadius;
	}
}