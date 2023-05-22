#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coords;

out vec2 TexCoords;
uniform mat4 model;

void main()
{
	TexCoords = tex_coords;
	gl_Position = model * vec4(pos, 0.0f, 1.0f);
}

#shader fragment
#version 330 core

uniform sampler2D texture_inventory;

in vec2 TexCoords;
out vec4 OutColor;

void main()
{
	OutColor = texture(texture_inventory, TexCoords);

	if (OutColor.a < 0.1f) {
		discard;
	}
}