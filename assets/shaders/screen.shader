#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_coords;

out vec2 TexCoords;

void main(){
	//The transformations passed to the shaders need to be pre-transformed
	TexCoords = tex_coords;
	//the main screen space is an orthogonal view while the world has a perspective view
	//if we render the world texture a little bit further back we wont see any differences
	//because the main renderpass is orthogonal, and we have the possibility to render some 
	//3d items from the ui, like the held block. This is why the z here is set to -1.0f
	gl_Position = vec4(pos, -1.0f, 1.0f);
}

#shader fragment
#version 330 core

in vec2 TexCoords;
uniform sampler2D texture_screen;
out vec4 OutColor;

void main(){
	OutColor = texture(texture_screen, TexCoords);
}

