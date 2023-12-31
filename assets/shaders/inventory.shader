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
uniform vec2 item_offsets[10];

in vec2 TexCoords;
out vec4 OutColor;

void main()
{
	vec2 offset = optional_texture_index != -1 ? TexCoords + item_offsets[optional_texture_index] : TexCoords;
	OutColor = texture(texture_inventory, offset);

	if (OutColor.a < 0.1f) {
		discard;
	}
}
