#version 450
layout(set = 0, binding = 0) uniform sampler2D glyphAtlas;
layout(location = 0) in vec2 local;
layout(location = 1) in vec2 size;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 parameters;
layout(location = 4) in vec2 uv;
layout(location = 0) out vec4 outputColor;
vec3 ToLinear(vec3 value)
{
	return mix(value / 12.92, pow((value + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), value));
}
void main()
{
	float coverage;
	if (parameters.z > 1.5)
	{
		vec4 pixel = texture(glyphAtlas, uv);
		float alpha = color.a * pixel.a;
		outputColor = vec4(ToLinear(color.rgb) * pixel.rgb * color.a, alpha);
		return;
	}
	else if (parameters.z > 0.5)
	{
		coverage = texture(glyphAtlas, uv).r;
	}
	else
	{
		float radius = min(parameters.x, min(size.x, size.y) * 0.5);
		vec2 q = abs(local - size * 0.5) - (size * 0.5 - radius);
		float distance = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
		float aa = max(fwidth(distance), 0.001);
		coverage = clamp(0.5 - distance / aa, 0.0, 1.0);
		if (parameters.y > 0.0)
			coverage *= clamp(0.5 + (distance + parameters.y) / aa, 0.0, 1.0);
	}
	float alpha = color.a * coverage;
	outputColor = vec4(ToLinear(color.rgb) * alpha, alpha);
}
