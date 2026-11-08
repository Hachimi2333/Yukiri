#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inLocal;
layout(location = 2) in vec2 inSize;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inParameters;
layout(location = 5) in vec2 inUv;
layout(push_constant) uniform Push { vec2 extent; } pushData;
layout(location = 0) out vec2 local;
layout(location = 1) out vec2 size;
layout(location = 2) out vec4 color;
layout(location = 3) out vec4 parameters;
layout(location = 4) out vec2 uv;
void main()
{
	gl_Position = vec4(inPosition / pushData.extent * 2.0 - 1.0, 0.0, 1.0);
	local = inLocal;
	size = inSize;
	color = inColor;
	parameters = inParameters;
	uv = inUv;
}
