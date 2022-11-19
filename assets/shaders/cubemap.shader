#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;

uniform mat4 model;
uniform mat4 proj;

out vec3 TexCoord;

void main()
{
	TexCoord = normalize(-pos);
	gl_Position = proj * model * vec4(pos, 1.0f);
}

#shader fragment
#version 330 core

in vec3 TexCoord;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
	FragColor = texture(skybox, TexCoord);
}