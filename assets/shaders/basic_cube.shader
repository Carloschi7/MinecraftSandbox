#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex_coords;

in mat4 model_matrix;
in uint tex_index;

uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoords;
flat out uint TexIndex;
out vec3 Norm;

void main()
{
	TexCoords = tex_coords;
	TexIndex = tex_index;
	Norm = normalize(norm);

	gl_Position = proj * view * model_matrix * vec4(pos, 1.0f);
}

#shader fragment
#version 330 core

uniform sampler2D texture_dirt;
uniform sampler2D texture_grass;
uniform sampler2D texture_sand;
uniform sampler2D texture_trunk;
uniform sampler2D texture_leaves;

uniform vec3 light_direction;

in vec2 TexCoords;
flat in uint TexIndex;
in vec3 Norm;

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
		return texture(texture_sand, TexCoords);
	case 3:
		return texture(texture_trunk, TexCoords);
	case 4:
		return texture(texture_leaves, TexCoords);
		//Darker versions
	case 256 + 0:
		return texture(texture_dirt, TexCoords) * 0.4f;
	case 256 + 1:
		return texture(texture_grass, TexCoords) * 0.4f;
	case 256 + 2:
		return texture(texture_sand, TexCoords) * 0.4f;
	case 256 + 3:
		return texture(texture_trunk, TexCoords) * 0.4f;
	case 256 + 4:
		return texture(texture_leaves, TexCoords) * 0.4f;
	}

	return vec4(0.0f);
}

void main()
{
	float diff = max(dot(Norm, -light_direction), 0.6f);
	OutColor = choose_tex(int(TexIndex)) * diff;
}