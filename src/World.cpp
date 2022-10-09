#include "World.h"

//Initializing a single block for now
World::World(const Window& game_window)
	:m_WorldStructure(game_window)
{
	m_Chunks.emplace_back(glm::vec3(0.0f));
}

void World::DrawRenderable() const
{
	GameDefs::RenderData draw_data = m_WorldStructure.GetRenderFrameInfo();

	for (const auto& chunk : m_Chunks)
		chunk.Draw(draw_data);
	
	
	//Drawing crossaim
	m_WorldStructure.RenderCrossaim();
}

void World::UpdateScene()
{
	GameDefs::ChunkBlockLogicData chunk_logic_data = m_WorldStructure.GetChunkBlockLogicData();
	for (auto& chunk : m_Chunks)
		chunk.BlockCollisionLogic(chunk_logic_data);

	m_WorldStructure.UpdateCamera();
}
