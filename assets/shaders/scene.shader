#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex_coords;

in vec3 model_pos;
in uint tex_index;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 light_space;

out vec2 TexCoords;
flat out uint TexIndex;
out vec3 Norm;
out vec4 LightSpacePos;

void main()
{
	TexCoords = tex_coords;
	TexIndex = tex_index;
	Norm = normalize(norm);

	//A translation matrix without other transformations is just an identity matrix
	//with the 3D position written in the first 3 slots of the fourth column
	mat4 model_matrix = mat4(
		vec4(1.0f, 0.0f, 0.0f, 0.0f), 	//Column 1
		vec4(0.0f, 1.0f, 0.0f, 0.0f), 	//Column 2 
		vec4(0.0f, 0.0f, 1.0f, 0.0f), 	//Column 3
		vec4(model_pos, 1.0f));		//Column 4

	//Scale selection
	if (int(TexIndex) >= 256)
	{
		float fact = 1.05f;

		mat4 scale_matrix = mat4(
			vec4(fact, 0.0f, 0.0f, 0.0f),
			vec4(0.0f, fact, 0.0f, 0.0f),
			vec4(0.0f, 0.0f, fact, 0.0f),
			vec4(0.0f, 0.0f, 0.0f, 1.0f));

		model_matrix = model_matrix * scale_matrix;
	}

	vec4 ws_pos = model_matrix * vec4(pos, 1.0f);
	LightSpacePos = light_space * ws_pos;
	gl_Position = proj * view * ws_pos;
}

#shader fragment
#version 330 core

uniform sampler2D texture_dirt;
uniform sampler2D texture_grass;
uniform sampler2D texture_sand;
uniform sampler2D texture_wood;
uniform sampler2D texture_wood_planks;
uniform sampler2D texture_leaves;
uniform sampler2D texture_crafting_table;

uniform sampler2D texture_depth;

in vec2 TexCoords;
flat in uint TexIndex;
in vec3 Norm;
in vec4 LightSpacePos;

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
		return texture(texture_wood, TexCoords);
	case 4:
		return texture(texture_wood_planks, TexCoords);
	case 5:
		return texture(texture_leaves, TexCoords);
	case 6:
		return texture(texture_crafting_table, TexCoords);
	case 256 + 0:
		return texture(texture_dirt, TexCoords) * 0.4f;
	case 256 + 1:
		return texture(texture_grass, TexCoords) * 0.4f;
	case 256 + 2:
		return texture(texture_sand, TexCoords) * 0.4f;
	case 256 + 3:
		return texture(texture_wood, TexCoords) * 0.4f;
	case 256 + 4:
		return texture(texture_wood_planks, TexCoords) * 0.4f;
	case 256 + 5:
		return texture(texture_leaves, TexCoords) * 0.4f;
	case 256 + 6:
		return texture(texture_crafting_table, TexCoords) * 0.4f;
	}

	return vec4(0.0f);
}

void main()
{
	vec3 light_direction = vec3(0.0f, -1.0f, 0.0f);

	float darkness_value = 0.4f;
	float dot_value = dot(Norm, -light_direction);
	float diff = max(dot_value, darkness_value);
	OutColor = choose_tex(int(TexIndex)) * diff;
	//If the fragment is out of the light space, discard the fragment
	if (LightSpacePos.x < -1.0f || LightSpacePos.y < -1.0f || LightSpacePos.x > 1.0f || LightSpacePos.y > 1.0f)
		return;

	//Only apply shadow if the surface is facing the light source to some degree,
	//otherwise rely only on dot computed diffused lighting
	if (dot_value > 0.0f) {
		//compute the [0.0f, 1.0f] vector equivalent
		vec3 equiv = (LightSpacePos.xyz + vec3(1.0f)) * 0.5f;
		float closest_depth = texture(texture_depth, equiv.xy).r;
		if (equiv.z - closest_depth > 0.001f)
			OutColor *= darkness_value;
	}
}
