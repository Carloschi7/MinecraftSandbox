#shader vertex
#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 lightSpace;

//Still instanced
in vec3 model_depth_pos;

void main()
{
	mat4 model_matrix = mat4(
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(model_depth_pos, 1.0f));

	gl_Position = lightSpace * model_matrix * vec4(position, 1.0f);
}

#shader fragment
#version 330 core

void main()
{
	gl_FragDepth = gl_FragCoord.z;
}