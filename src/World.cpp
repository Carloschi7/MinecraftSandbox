#include "World.h"

//Initializing a single block for now
World::World(const Window& game_window)
	:m_WorldStructure(game_window), m_LastPos(0.0f)
{
	using namespace GameDefs;

	for (int32_t i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (int32_t j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement)
			m_Chunks.emplace_back(this, glm::vec2(float(i), float(j)));

	auto add_spawnable_chunk = [&](const glm::vec3& pos)
	{
		glm::vec3 res = pos + Chunk::GetHalfWayVector();
		m_SpawnableChunks.emplace_back(res);
	};

	//Init spawnable chunks
	for (auto& chunk : m_Chunks)
	{
		const glm::vec2& chunk_pos = chunk.GetChunkOrigin();

		if (!IsChunk(chunk, ChunkLocation::PLUS_X).has_value())
			add_spawnable_chunk(glm::vec3(g_SpawnerEnd, 0.0f, chunk_pos.y));
		if (!IsChunk(chunk, ChunkLocation::MINUS_X).has_value())
			add_spawnable_chunk(glm::vec3(g_SpawnerBegin - g_SpawnerIncrement, 0.0f, chunk_pos.y));
		if (!IsChunk(chunk, ChunkLocation::PLUS_Z).has_value())
			add_spawnable_chunk(glm::vec3(chunk_pos.x, 0.0f, g_SpawnerEnd));
		if (!IsChunk(chunk, ChunkLocation::MINUS_Z).has_value())
			add_spawnable_chunk(glm::vec3(chunk_pos.x, 0.0f, g_SpawnerBegin - g_SpawnerIncrement));

		chunk.InitBlockNormals();
	}
}

void World::DrawRenderable() const
{
	//Render skybox
	m_WorldStructure.RenderSkybox();

	GameDefs::RenderData draw_data = m_WorldStructure.GetRenderFrameInfo();
	GameDefs::ChunkLogicData logic_data = m_WorldStructure.GetChunkLogicData();
	//Initialising block shader uniform
	m_WorldStructure.UniformRenderInit(draw_data, GlCore::BlockStructure::GetShader());
	for (const auto& chunk : m_Chunks)
		if(chunk.IsChunkRenderable(logic_data) && chunk.IsChunkVisible(logic_data))
			chunk.Draw(draw_data);

	//Resetting selection
	for (const auto& chunk : m_Chunks)
		chunk.SetBlockSelected(false);
	
	//Drawing crossaim
	m_WorldStructure.RenderCrossaim();
}

void World::UpdateScene()
{
	GameDefs::ChunkLogicData chunk_logic_data = m_WorldStructure.GetChunkLogicData();

	//Chunk dynamic spawning
	for (uint32_t i = 0; i < m_SpawnableChunks.size(); i++)
	{
		const glm::vec3& vec = m_SpawnableChunks[i];
		glm::vec2 vec_2d(vec.x, vec.z);
		glm::vec2 camera_2d(chunk_logic_data.camera_position.x, chunk_logic_data.camera_position.z);
		if (glm::length(vec_2d - camera_2d) < GameDefs::g_ChunkSpawningDistance)
		{
			glm::vec3 origin_chunk_pos = vec - Chunk::GetHalfWayVector();
			glm::vec2 chunk_pos = { origin_chunk_pos.x, origin_chunk_pos.z };
			m_Chunks.emplace_back(this, chunk_pos);
			auto& this_chunk = m_Chunks.back();
			this_chunk.InitBlockNormals();
			
			//Removing previously visible normals from old chunks && update local chunks
			//The m_Chunk.size() - 1 index is the effective index of the newly pushed chunk
			if (auto opt = this_chunk.GetLoadedChunk(GameDefs::ChunkLocation::PLUS_X); opt.has_value())
			{
				auto& neighbor_chunk = m_Chunks[opt.value()];
				neighbor_chunk.SetLoadedChunk(GameDefs::ChunkLocation::MINUS_X, m_Chunks.size() - 1);
				neighbor_chunk.RemoveBorderNorm(glm::vec3(-1.0f, 0.0f, 0.0f));
			}
			if (auto opt = this_chunk.GetLoadedChunk(GameDefs::ChunkLocation::MINUS_X); opt.has_value())
			{
				auto& neighbor_chunk = m_Chunks[opt.value()];
				neighbor_chunk.SetLoadedChunk(GameDefs::ChunkLocation::PLUS_X, m_Chunks.size() - 1);
				neighbor_chunk.RemoveBorderNorm(glm::vec3(1.0f, 0.0f, 0.0f));
			}
			if (auto opt = this_chunk.GetLoadedChunk(GameDefs::ChunkLocation::PLUS_Z); opt.has_value())
			{
				auto& neighbor_chunk = m_Chunks[opt.value()];
				neighbor_chunk.SetLoadedChunk(GameDefs::ChunkLocation::MINUS_Z, m_Chunks.size() - 1);
				neighbor_chunk.RemoveBorderNorm(glm::vec3(0.0f, 0.0f, -1.0f));
			}
			if (auto opt = this_chunk.GetLoadedChunk(GameDefs::ChunkLocation::MINUS_Z); opt.has_value())
			{
				auto& neighbor_chunk = m_Chunks[opt.value()];
				neighbor_chunk.SetLoadedChunk(GameDefs::ChunkLocation::PLUS_Z, m_Chunks.size() - 1);
				neighbor_chunk.RemoveBorderNorm(glm::vec3(0.0f, 0.0f, 1.0f));
			}

			//Pushing new spawnable vectors
			if (IsPushable(this_chunk, GameDefs::ChunkLocation::PLUS_X, vec + glm::vec3(16.0f, 0.0f, 0.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(16.0f, 0.0f, 0.0f));
			if (IsPushable(this_chunk, GameDefs::ChunkLocation::MINUS_X, vec + glm::vec3(-16.0f, 0.0f, 0.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(-16.0f, 0.0f, 0.0f));
			if (IsPushable(this_chunk, GameDefs::ChunkLocation::PLUS_Z, vec + glm::vec3(0.0f, 0.0f, 16.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, 16.0f));
			if (IsPushable(this_chunk, GameDefs::ChunkLocation::MINUS_Z, vec + glm::vec3(0.0f, 0.0f, -16.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, -16.0f));

			//Remove the just spawned chunk from the spawnable list
			m_SpawnableChunks.erase(m_SpawnableChunks.begin() + i);
			
			break;
		}
	}

	//Block Selection & normal updating

	float nearest_selection = INFINITY;
	int32_t involved_chunk = static_cast<uint32_t>(-1);
	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		auto& chunk = m_Chunks[i];
		if (!chunk.IsChunkRenderable(chunk_logic_data) || !chunk.IsChunkVisible(chunk_logic_data))
			continue;

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

std::optional<uint32_t> World::IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.GetChunkOrigin();
	auto find_alg = [&](const glm::vec2& pos) ->std::optional<uint32_t>
	{
		for (uint32_t i = 0; i < m_Chunks.size(); i++)
		{
			if (m_Chunks[i].GetChunkOrigin() == pos)
				return i;
		}

		return std::nullopt;
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

	return std::nullopt;
}

Chunk& World::GetChunk(uint32_t index)
{
	if (index >= m_Chunks.size())
		throw std::runtime_error("Vector index out of bounds");

	return m_Chunks[index];
}

bool World::IsPushable(const Chunk& chunk, const GameDefs::ChunkLocation& cl, const glm::vec3& vec)
{
	auto internal_pred = [&](const glm::vec3& internal_vec) {return internal_vec == vec; };

	return !IsChunk(chunk, cl).has_value() && 
		std::find_if(m_SpawnableChunks.begin(), m_SpawnableChunks.end(), internal_pred) == m_SpawnableChunks.end();
}

