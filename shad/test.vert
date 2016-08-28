#version 450

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec3 i_Color;

out gl_PerVertex {
	vec4 gl_Position;
};
layout(location = 0) out vec4 v_Color;

void main() {
	gl_Position = vec4(i_Position, 0, 1);
	v_Color = vec4(i_Color, 0);
}
