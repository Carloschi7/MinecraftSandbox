#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coords;

out vec2 TexCoords;
uniform mat4 proj;
uniform mat4 model;

void main()
{
	TexCoords = tex_coords;
	gl_Position = proj * model * vec4(pos, 0.0f, 1.0f);
}

#shader fragment
#version 330 core

uniform sampler2D texture_inventory;
uniform int optional_texture_index;

vec2 get_offset(int index)
{
	//blocks per row
	float bpr = 16.0f;
	switch (index)
	{
	case 0:
		//Dirt
		return vec2(0.0f, 0.0f);
	case 1:
		//Grass
		return vec2(4.0f / bpr, 0.0f);
	case 2:
		//Sand
		return vec2(8.0f / bpr, 0.0f);
	case 3:
		//Wood
		return vec2(4.0f / bpr, 1.0f / bpr);
	case 4:
		//Wood planks
		return vec2(8.0f / bpr, 1.0f / bpr);
	case 5:
		//Leaves
		return vec2(4.0f / bpr, 0.0f);
	case 6:
		//Crafting table
		return vec2(12.0f / bpr, 1.0f / bpr);
	case 7:
		//Wood stick
		return vec2(0.0f, 2.0f / bpr);
	case 8:
		//Wood pickaxe
		return vec2(4.0f / bpr, 2.0f / bpr);
	}

	return vec2(-1.0f);
}

in vec2 TexCoords;
out vec4 OutColor;

void main()
{
	vec2 offset = optional_texture_index != -1 ? TexCoords + get_offset(optional_texture_index) : TexCoords;
	OutColor = texture(texture_inventory, offset);

	if (OutColor.a < 0.1f) {
		discard;
	}
}