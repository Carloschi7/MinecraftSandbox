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
uniform sampler2D global_texture;
uniform vec2 item_offsets[10];

out vec4 OutColor;

void main() 
{
	OutColor = texture(global_texture, TexCoords + item_offsets[drop_texture_index]);
	//OutColor = vec4(get_offset(drop_texture_index), 0.0f, 1.0f);
}

