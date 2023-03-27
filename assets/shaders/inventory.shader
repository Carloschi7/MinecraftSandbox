#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coords;

out vec2 TexCoords;

void main()
{
	TexCoords = tex_coords;
	gl_Position = vec4(pos, 0.0f, 1.0f);
}

#shader fragment

uniform sampler2D texture_inventory;

in vec2 TexCoords;
out vec4 OutColor;

void main()
{
	OutColor = texture(texture_inventory, TexCoords);
}