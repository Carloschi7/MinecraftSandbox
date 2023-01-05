#include "World.h"
#include "Renderer.h"
#include <atomic>

//Initializing a single block for now
World::World()
	: m_LastPos(0.0f)
{
	using namespace Gd;

	//Init world seed
	m_WorldSeed.seed_value = 1;
	PerlNoise::InitSeedMap(m_WorldSeed);

	for (int32_t i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (int32_t j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement)
			m_Chunks.emplace_back(this, glm::vec2(float(i), float(j)));

	HandleSectionData();
	//Set safe iterable size for the rendering thread(set in all cases by default)
	m_SafeChunkSize = m_Chunks.size();

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

World::~World()
{
}

void World::DrawRenderable()
{
	//Render skybox
	m_WorldStructure.RenderSkybox();

	Gd::RenderData draw_data = GlCore::WorldStructure::GetRenderFrameInfo();
	Gd::ChunkLogicData logic_data = GlCore::WorldStructure::GetChunkLogicData();
	
	m_WorldStructure.UniformRenderInit(draw_data);

	uint32_t ch = Gd::g_SelectedChunk.load();
	//Setting selected block index, which will be used only by the owning chunk
	Chunk::s_InternalSelectedBlock = Gd::g_SelectedBlock.load();
	
	//Asserting chunks are not getting serialized if
	//we are in multithreading mode (temporary solution)
	if constexpr (GlCore::g_MultithreadedRendering)
		while (m_ChunkMemoryOperations) {}

	std::size_t size = GlCore::g_MultithreadedRendering ? m_SafeChunkSize.load() : m_Chunks.size();
	for (uint32_t i = 0; i < size; i++)
	{
		const auto& chunk = m_Chunks[i];
		if (chunk.IsChunkRenderable(logic_data) && chunk.IsChunkVisible(logic_data))
			chunk.Draw(draw_data, ch == i);
	}

	//Drawing crossaim
	m_WorldStructure.RenderCrossaim();
}

void World::UpdateScene()
{
	Gd::ChunkLogicData chunk_logic_data = GlCore::WorldStructure::GetChunkLogicData();

	//Chunk dynamic spawning
	glm::vec2 camera_2d(chunk_logic_data.camera_position.x, chunk_logic_data.camera_position.z);
	for (uint32_t i = 0; i < m_SpawnableChunks.size(); i++)
	{
		const glm::vec3& vec = m_SpawnableChunks[i];
		glm::vec2 vec_2d(vec.x, vec.z);
		if (glm::length(vec_2d - camera_2d) < Gd::g_ChunkSpawningDistance)
		{
			glm::vec3 origin_chunk_pos = vec - Chunk::GetHalfWayVector();
			glm::vec2 chunk_pos = { origin_chunk_pos.x, origin_chunk_pos.z };

#ifdef STRONG_THREAD_SAFETY
			bool needs_lock = m_Chunks.size() == m_Chunks.capacity();
			if (needs_lock)
				if constexpr (GlCore::g_MultithreadedRendering)
					m_Chunks.lock();
#endif
			m_Chunks.emplace_back(this, chunk_pos);
			m_SafeChunkSize++;
			HandleSectionData();
#ifdef STRONG_THREAD_SAFETY
			if (needs_lock)
				if constexpr (GlCore::g_MultithreadedRendering)
					m_Chunks.unlock();
#endif
			auto& this_chunk = m_Chunks.back();
			this_chunk.InitBlockNormals();
			
			//Removing previously visible normals from old chunks && update local chunks
			//The m_Chunk.size() - 1 index is the effective index of the newly pushed chunk
			if (auto opt = this_chunk.GetLoadedChunk(Gd::ChunkLocation::PLUS_X); opt.has_value())
			{
				auto& neighbor_chunk = GetChunk(opt.value());
				neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::MINUS_X, this_chunk.Index());
			}
			if (auto opt = this_chunk.GetLoadedChunk(Gd::ChunkLocation::MINUS_X); opt.has_value())
			{
				auto& neighbor_chunk = GetChunk(opt.value());
				neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::PLUS_X, this_chunk.Index());
			}
			if (auto opt = this_chunk.GetLoadedChunk(Gd::ChunkLocation::PLUS_Z); opt.has_value())
			{
				auto& neighbor_chunk = GetChunk(opt.value());
				neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::MINUS_Z, this_chunk.Index());
			}
			if (auto opt = this_chunk.GetLoadedChunk(Gd::ChunkLocation::MINUS_Z); opt.has_value())
			{
				auto& neighbor_chunk = GetChunk(opt.value());
				neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::PLUS_Z, this_chunk.Index());
			}

			//Pushing new spawnable vectors
			if (IsPushable(this_chunk, Gd::ChunkLocation::PLUS_X, vec + glm::vec3(16.0f, 0.0f, 0.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(16.0f, 0.0f, 0.0f));
			if (IsPushable(this_chunk, Gd::ChunkLocation::MINUS_X, vec + glm::vec3(-16.0f, 0.0f, 0.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(-16.0f, 0.0f, 0.0f));
			if (IsPushable(this_chunk, Gd::ChunkLocation::PLUS_Z, vec + glm::vec3(0.0f, 0.0f, 16.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, 16.0f));
			if (IsPushable(this_chunk, Gd::ChunkLocation::MINUS_Z, vec + glm::vec3(0.0f, 0.0f, -16.0f)))
				m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, -16.0f));

			//Remove the just spawned chunk from the spawnable list
			m_SpawnableChunks.erase(m_SpawnableChunks.begin() + i);
			
			break;
		}
	}

	//Block Selection & normal updating
	HandleSelection(chunk_logic_data);
	m_LastPos = chunk_logic_data.camera_position;

	//TODO Serialization (Test)
	for (Gd::SectionData& data : m_SectionsData)
	{
		//Serialization zone
		if (data.loaded && glm::length(camera_2d - data.central_position) > 750.0f)
		{
			SerializeSector(data.index);
			std::cout << "Serialize:" << m_Chunks.size() << std::endl;
			data.loaded = false;
		}

		//Deserialization zone
		if (!data.loaded && glm::length(camera_2d - data.central_position) < 650.0f)
		{
			DeserializeSector(data.index);
			std::cout << "Deserialize:" << m_Chunks.size() << std::endl;
			data.loaded = true;
		}
	}
}

void World::HandleSelection(const Gd::ChunkLogicData& ld)
{
	float nearest_selection = INFINITY;
	int32_t involved_chunk = static_cast<uint32_t>(-1);
	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		auto& chunk = m_Chunks[i];
		if (!chunk.IsChunkRenderable(ld) || !chunk.IsChunkVisible(ld))
			continue;

		float current_selection = chunk.BlockCollisionLogic(ld);

		//We update blocks drawing conditions only if we move or if we break blocks
		if (ld.camera_position != m_LastPos || ld.mouse_input.left_click)
			chunk.UpdateBlocks(ld);

		if (current_selection < nearest_selection)
		{
			nearest_selection = current_selection;
			involved_chunk = i;
		}
	}

	//Setting from which chunk the selectedd block comes from
	Gd::g_SelectedChunk = involved_chunk;
	if (involved_chunk != static_cast<uint32_t>(-1))
	{
		Gd::g_SelectedBlock = m_Chunks[involved_chunk].LastSelectedBlock();
	}
}

void World::HandleSectionData()
{
	//Load pushed sections
	for (auto& obj : Gd::g_PushedSections)
	{
		//Checking that it was now pushed yet
		if (std::find_if(m_SectionsData.begin(), m_SectionsData.end(),
			[&](const auto& elem) {return elem.index == obj; }) == m_SectionsData.end())
		{
			Gd::SectionData sd{ obj, SectionCentralPosFrom(obj), true };
			m_SectionsData.push_back(sd);
		}
	}

	Gd::g_PushedSections.clear();
}

std::optional<uint32_t> World::IsChunk(const Chunk& chunk, const Gd::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.GetChunkOrigin();
	auto find_alg = [&](const glm::vec2& pos) ->std::optional<uint32_t>
	{
		for (uint32_t i = 0; i < m_Chunks.size(); i++)
		{
			if (m_Chunks[i].GetChunkOrigin() == pos)
				return m_Chunks[i].Index();
		}

		return std::nullopt;
	};

	switch (cl)
	{
	case Gd::ChunkLocation::PLUS_X:
		return find_alg(origin + glm::vec2(16.0f, 0.0f));
	case Gd::ChunkLocation::MINUS_X:
		return find_alg(origin - glm::vec2(16.0f, 0.0f));
	case Gd::ChunkLocation::PLUS_Z:
		return find_alg(origin + glm::vec2(0.0f, 16.0f));
	case Gd::ChunkLocation::MINUS_Z:
		return find_alg(origin - glm::vec2(0.0f, 16.0f));
	}

	return std::nullopt;
}

Chunk& World::GetChunk(uint32_t index)
{
	auto iter = std::find_if(m_Chunks.begin(), m_Chunks.end(), 
		[index](const Chunk& c) {return c.Index() == index; });

	if (iter == m_Chunks.end())
		throw std::runtime_error("Chunk element with this index not found");

	return *iter;
}

Gd::WorldSeed& World::Seed()
{
	return m_WorldSeed;
}

const Gd::WorldSeed& World::Seed() const
{
	return m_WorldSeed;
}

void World::SerializeSector(uint32_t index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Gd::g_SerializedFileFormat, "wb");
	uint32_t serialized_chunks = 0;

	//(When multithreading) Advertise the render thread m_Chunks 
	//is undergoing some heavy changes
	if constexpr (GlCore::g_MultithreadedRendering)
	{
		m_ChunkMemoryOperations = true;

		//Rearrange all the elements so that the ones that need to be serialized are at the end
		auto iter = std::partition(m_Chunks.begin(), m_Chunks.end(), [&](const Chunk& ch) {return ch.SectorIndex() != index; });
		//Determine safe iteration range for the renderer thread
		m_SafeChunkSize = static_cast<uint32_t>(iter - m_Chunks.begin());

		m_ChunkMemoryOperations = false;
	}

	//Number of chunks at the beginning (leave blank for now)
	sz.Serialize<uint32_t>(0);
	for (auto iter = m_Chunks.begin(); iter != m_Chunks.end();)
	{
		Chunk& chunk = *iter;
		if (chunk.SectorIndex() == index)
		{
			chunk & sz;
			serialized_chunks++;
			iter = m_Chunks.erase(iter);
			continue;
		}
		++iter;
	}
	
	//Write the amount of chunks serialized
	sz.Seek(0);
	sz& serialized_chunks;

	m_SafeChunkSize = m_Chunks.size();
}

void World::DeserializeSector(uint32_t index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Gd::g_SerializedFileFormat, "rb");

	if constexpr (GlCore::g_MultithreadedRendering)
	{
		m_ChunkMemoryOperations = true;

		uint32_t deser_size = 0;
		sz% deser_size;
		m_Chunks.reserve(m_Chunks.size() + deser_size);

		m_ChunkMemoryOperations = false;
	}
	else
	{
		//Not used
		uint32_t deser_size = 0;
		sz% deser_size;
	}

	while (!sz.Eof())
	{
		m_Chunks.emplace_back(this, sz, index);
		m_SafeChunkSize++;
	}
}

bool World::IsPushable(const Chunk& chunk, const Gd::ChunkLocation& cl, const glm::vec3& vec)
{
	auto internal_pred = [&](const glm::vec3& internal_vec) {return internal_vec == vec; };

	return !IsChunk(chunk, cl).has_value() && 
		std::find_if(m_SpawnableChunks.begin(), m_SpawnableChunks.end(), internal_pred) == m_SpawnableChunks.end();
}

glm::vec2 World::SectionCentralPosFrom(uint32_t index)
{
	//extract the 2 u16
	int16_t coords[2];
	std::memcpy(coords, &index, sizeof(uint32_t));

	glm::vec2 pos = glm::vec2(coords[0], coords[1]) * Gd::g_SectionDimension;
	pos.x += Gd::g_SectionDimension * 0.5f;
	pos.y += Gd::g_SectionDimension * 0.5f;

	return pos;
}

