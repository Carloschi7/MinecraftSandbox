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

uniform sampler2D global_texture;
uniform sampler2D texture_depth;

in vec2 TexCoords;
flat in uint TexIndex;
in vec3 Norm;
in vec4 LightSpacePos;

out vec4 OutColor;

vec2 get_offset(int index)
{	
	//blocks per row
	float bpr = 16.0f;
	index -= index >= 256 ? 256 : 0;
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
		return vec2(12.0f / bpr, 0.0f);
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

void main()
{
	vec3 light_direction = vec3(0.0f, -1.0f, 0.0f);

	float darkness_value = 0.4f;
	float dot_value = dot(Norm, -light_direction);
	float diff = max(dot_value, darkness_value);
	OutColor = texture(global_texture, TexCoords + get_offset(int(TexIndex))) * diff;
	OutColor *= int(TexIndex) >= 256 ? 0.4f : 1.0f;

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
