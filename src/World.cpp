#include "World.h"

//Initializing a single block for now
World::World(const Window& game_window)
	:m_WorldStructure(game_window), m_LastPos(0.0f)
{
	for (int32_t i = -64; i < 64; i += 16)
		for (int32_t j = -64; j < 64; j += 16)
			m_Chunks.emplace_back(this, glm::vec2(float(i), float(j)));

	for (auto& chunk : m_Chunks)
		chunk.InitBlockNormals();
}

void World::DrawRenderable() const
{
	//Render skybox
	m_WorldStructure.RenderSkybox();

	GameDefs::RenderData draw_data = m_WorldStructure.GetRenderFrameInfo();
	//Initialising block shader uniform
	m_WorldStructure.UniformRenderInit(draw_data, GlCore::BlockStructure::GetShader());
	for (const auto& chunk : m_Chunks)
		chunk.Draw(draw_data);

	//Resetting selection
	for (const auto& chunk : m_Chunks)
		chunk.SetBlockSelected(false);
	
	//Drawing crossaim
	m_WorldStructure.RenderCrossaim();
}

void World::UpdateScene()
{
	GameDefs::ChunkBlockLogicData chunk_logic_data = m_WorldStructure.GetChunkBlockLogicData();

	float nearest_selection = INFINITY;
	int32_t involved_chunk = static_cast<uint32_t>(-1);
	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		auto& chunk = m_Chunks[i];
		float current_selection = chunk.BlockCollisionLogic(chunk_logic_data);
		
		//We update blocks drawing conditions only if we move or if we break blocks
		if (chunk_logic_data.camera_position != m_LastPos || chunk_logic_data.mouse_input.left_click)
			chunk.UpdateBlocks(chunk_logic_data);
		
		if (current_selection < nearest_selection)
		{
			nearest_selection = current_selection;
			involved_chunk = i;
		}
	}

	if(involved_chunk != static_cast<uint32_t>(-1))
		m_Chunks[involved_chunk].SetBlockSelected(true);

	m_LastPos = chunk_logic_data.camera_position;
	m_WorldStructure.UpdateCamera();
}

std::vector<Chunk>::iterator World::IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.GetChunkOrigin();
	auto find_alg = [&](const glm::vec2& pos) ->std::vector<Chunk>::iterator
	{
		return std::find_if(m_Chunks.begin(), m_Chunks.end(), [pos](const Chunk& c)
			{
				return c.GetChunkOrigin() == pos;
			});
	};

	switch (cl)
	{
	case GameDefs::ChunkLocation::PLUS_X:
		return find_alg(origin + glm::vec2(16.0f, 0.0f));
	case GameDefs::ChunkLocation::MINUS_X:
		return find_alg(origin - glm::vec2(16.0f, 0.0f));
	case GameDefs::ChunkLocation::PLUS_Z:
		return find_alg(origin + glm::vec2(0.0f, 16.0f));
	case GameDefs::ChunkLocation::MINUS_Z:
		return find_alg(origin - glm::vec2(0.0f, 16.0f));
	}

	return m_Chunks.end();
}

std::vector<Chunk>::iterator World::ChunkCend()
{
	return m_Chunks.end();
}

