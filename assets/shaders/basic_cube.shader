#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex_coords;

layout(std140) uniform OnFrequentUpdate{
	mat4 model;
} fu;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform bool update_with_ub;

out vec2 TexCoords;
out vec3 Norm;

void main()
{
	TexCoords = tex_coords;
	Norm = normalize(norm);

	if (!update_with_ub)
		gl_Position = proj * view * model * vec4(pos, 1.0f);
	else
		gl_Position = proj * view * fu.model * vec4(pos, 1.0f);
}

#shader fragment
#version 330 core

uniform sampler2D diffuse_texture;
uniform vec3 light_direction;

in vec2 TexCoords;
in vec3 Norm;

out vec4 OutColor;

void main()
{
	float diff = max(dot(Norm, -light_direction), 0.6f);
	OutColor = texture(diffuse_texture, TexCoords) * diff;
}