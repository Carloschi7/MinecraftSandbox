#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;

uniform mat4 model;

void main()
{
	gl_Position = model * vec4(pos, 1.0f, 1.0f);
}

#shader fragment
#version 330 core

out vec4 OutColor;

void main()
{
	OutColor = vec4(0.7f);
}
