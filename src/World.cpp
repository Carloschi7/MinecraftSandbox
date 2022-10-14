#include "World.h"

//Initializing a single block for now
World::World(const Window& game_window)
	:m_WorldStructure(game_window)
{
	m_Chunks.emplace_back(this, glm::vec3(0.0f));
	m_Chunks.emplace_back(this, glm::vec3(16.0f, 0.0f, 0.0f));

	for (auto& chunk : m_Chunks)
		chunk.InitBlockNormals();
}

void World::DrawRenderable() const
{
	//Render skybox
	m_WorldStructure.RenderSkybox();

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

std::vector<Chunk>::iterator World::IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl)
{
	const glm::vec3& origin = chunk.GetChunkOrigin();
	auto find_alg = [&](const glm::vec3& pos) ->std::vector<Chunk>::iterator
	{
		return std::find_if(m_Chunks.begin(), m_Chunks.end(), [pos](const Chunk& c)
			{
				return c.GetChunkOrigin() == pos;
			});
	};

	switch (cl)
	{
	case GameDefs::ChunkLocation::PLUS_X:
		return find_alg(origin + glm::vec3(16.0f, 0.0f, 0.0f));
	case GameDefs::ChunkLocation::MINUS_X:
		return find_alg(origin - glm::vec3(16.0f, 0.0f, 0.0f));
	case GameDefs::ChunkLocation::PLUS_Z:
		return find_alg(origin + glm::vec3(0.0f, 0.0f, 16.0f));
	case GameDefs::ChunkLocation::MINUS_Z:
		return find_alg(origin - glm::vec3(0.0f, 0.0f, 16.0f));
	}

	return m_Chunks.end();
}

std::vector<Chunk>::iterator World::ChunkCend()
{
	return m_Chunks.end();
}

