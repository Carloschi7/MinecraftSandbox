#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex_coords;

in vec3 model_pos;

uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoords;
out vec3 Norm;

void main()
{
	TexCoords = tex_coords;
	Norm = normalize(norm);

	mat4 model_matrix = mat4(
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(model_pos, 1.0f));

	gl_Position = proj * view * model_matrix * vec4(pos, 1.0f);
}

#shader fragment

uniform sampler2D texture_water;

in vec2 TexCoords;
in vec3 Norm;
out vec4 OutColor;

void main()
{
	OutColor = texture(texture_water, TexCoords);
	//OutColor = vec4(0.3f, 0.4f, 0.1f, 1.0f);
}