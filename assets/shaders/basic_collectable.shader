#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex_coords;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoords;

void main() {
	TexCoords = tex_coords;
	gl_Position = proj * view * model * vec4(pos, 1.0f);
}

#shader fragment
#version 330 core

in vec2 TexCoords;

uniform int drop_texture_index;
uniform sampler2D texture_dirt;
uniform sampler2D texture_grass;
uniform sampler2D texture_sand;
uniform sampler2D texture_trunk;
uniform sampler2D texture_leaves;

out vec4 OutColor;

vec4 choose_tex(int index)
{
	switch (index)
	{
	case 0:
		return texture(texture_dirt, TexCoords);
	case 1:
		return texture(texture_grass, TexCoords);
	case 2:
		//return vec4(texture(texture_depth, TexCoords).r);
		return texture(texture_sand, TexCoords);
	case 3:
		return texture(texture_trunk, TexCoords);
	case 4:
		return texture(texture_leaves, TexCoords);
	}

	return vec4(0.0f);
}

void main() {
	OutColor = choose_tex(drop_texture_index);
}