#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex_coords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoords;

void main()
{
	TexCoords = tex_coords;
	gl_Position = proj * view * model * vec4(pos, 1.0f);
}

#shader fragment
#version 330 core

uniform sampler2D diffuse_texture;
uniform bool entity_selected;

in vec2 TexCoords;
out vec4 OutColor;

void main()
{
	if (entity_selected)
		OutColor = texture(diffuse_texture, TexCoords) + vec4(vec3(0.2f), 1.0f);
	else
		OutColor = texture(diffuse_texture, TexCoords);
}