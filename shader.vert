#version 450

const vec2 vertices[] = {
	vec2(  0.0,                 0.5  ),
	vec2( -0.4330127018922193, -0.25 ),
	vec2(  0.4330127018922193, -0.25 )
};

const vec3 colors[] = {
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
};

layout( push_constant ) uniform Constants {
	mat2 transform;
} constants;

layout(location = 0) out vec3 color;

void main() {
	gl_Position = vec4(constants.transform * vertices[gl_VertexIndex], 0.0, 1.0);
	color = colors[gl_VertexIndex];
	gl_Position.y *= -1.0;
}
