#include "World.h"

//Initializing a single block for now
World::World(Camera& world_cam)
	:m_Block(glm::vec3(0.0f), GameDefs::BlockType::DIRT), m_WorldStructure(world_cam)
{
	
}

void World::DrawRenderable() const
{
	//Loading camera MVP to the shader
	m_WorldStructure.SetShaderCameraMVP(*m_Block.GetBlockShader());

	m_Block.Draw();
}

void World::UpdateScene()
{
	m_WorldStructure.UpdateCamera();
}
