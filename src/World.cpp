#include "World.h"
#include "Renderer.h"
#include <atomic>

//Initializing a single block for now
World::World()
	: m_State(GlCore::State::GetState()), m_LastPos(0.0f)
{
	using namespace Gd;

	//Init world seed
	m_WorldSeed.seed_value = 1;
	PerlNoise::InitSeedMap(m_WorldSeed);

	for (int32_t i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (int32_t j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement)
			m_Chunks.emplace_back(std::make_shared<Chunk>(*this, glm::vec2(float(i), float(j))));

	HandleSectionData();
	//Set safe iterable size for the rendering thread(set in all cases by default)
	m_SafeChunkSize = m_Chunks.size();

	auto add_spawnable_chunk = [&](const glm::vec3& pos)
	{
		glm::vec3 res = pos + Chunk::GetHalfWayVector();
		m_SpawnableChunks.emplace_back(res);
	};

	//Init spawnable chunks
	for (auto& chunk_ptr : m_Chunks)
	{
		auto& chunk = *chunk_ptr;
		const glm::vec2& chunk_pos = chunk.GetChunkOrigin();

		if (!IsChunk(chunk, ChunkLocation::PlusX).has_value())
			add_spawnable_chunk(glm::vec3(g_SpawnerEnd, 0.0f, chunk_pos.y));
		if (!IsChunk(chunk, ChunkLocation::MinusX).has_value())
			add_spawnable_chunk(glm::vec3(g_SpawnerBegin - g_SpawnerIncrement, 0.0f, chunk_pos.y));
		if (!IsChunk(chunk, ChunkLocation::PlusZ).has_value())
			add_spawnable_chunk(glm::vec3(chunk_pos.x, 0.0f, g_SpawnerEnd));
		if (!IsChunk(chunk, ChunkLocation::MinusZ).has_value())
			add_spawnable_chunk(glm::vec3(chunk_pos.x, 0.0f, g_SpawnerBegin - g_SpawnerIncrement));

		chunk.InitGlobalNorms();
	}

	//Set proj matrix, won't vary for now in the app
	m_WorldStructure.UniformProjMatrix();

	//Load water texture
	auto& textures = GlCore::LoadGameTextures();

	uint32_t water_binding = static_cast<uint32_t>(Gd::TextureBinding::TextureWater);
	textures[water_binding].Bind(water_binding);
	m_State.WaterShader()->Uniform1i(water_binding, "texture_water");
}

World::~World()
{
}

void World::Render()
{
	//Draw to depth framebuffer
	auto& window = m_State.GameWindow();
	auto& camera = m_State.GameCamera();
	auto& block_vm = m_State.BlockVM();
	auto& depth_vm = m_State.DepthVM();

	//Static buffers where render data is being sent to, 
	//to avoid making too many drawcalls
	static std::unique_ptr<glm::vec3[]> position_buffer;
	static std::unique_ptr<uint32_t[]> texture_buffer;
	uint32_t count = 0;

	//Populate the buffers the first iteration
	if (position_buffer == nullptr)
	{
		position_buffer = std::make_unique<glm::vec3[]>(GlCore::g_MaxRenderedObjCount);
		texture_buffer = std::make_unique<uint32_t[]>(GlCore::g_MaxRenderedObjCount);
	}

	bool depth_buffer_needs_update = Gd::g_BlockDestroyed || glm::length(m_LastPos - camera.GetPosition()) > 10.0f;
	if (depth_buffer_needs_update)
	{
		//Reset state
		Gd::g_BlockDestroyed = false;
		m_LastPos = camera.GetPosition();

		glViewport(0, 0, GlCore::g_DepthMapWidth, GlCore::g_DepthMapHeight);
		m_State.DepthFramebuffer()->Bind();
		Window::ClearScreen(GL_DEPTH_BUFFER_BIT);

		//Update depth framebuffer
		m_WorldStructure.UpdateShadowFramebuffer();

		for (uint32_t i = 0; i < MC_CHUNK_SIZE; i++)
		{
			//Interrupt for a moment if m_Chunks is being resized by the logic thread
			std::shared_ptr<Chunk> chunk = m_Chunks[i];
			if (chunk->IsChunkRenderable() && chunk->IsChunkVisibleByShadow())
				chunk->ForwardRenderableData(position_buffer.get(), texture_buffer.get(), count, true);
		}

		//Render any leftover data
		GlCore::DispatchDepthRendering(position_buffer.get(), count);

		uint32_t depth_binding = static_cast<uint32_t>(Gd::TextureBinding::TextureDepth);
		m_State.DepthFramebuffer()->BindFrameTexture(depth_binding);
		m_State.BlockShader()->Uniform1i(depth_binding, "texture_depth");

		glViewport(0, 0, window.Width(), window.Height());
		FrameBuffer::BindDefault();
	}
	Window::ClearScreen(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Render skybox
	m_WorldStructure.RenderSkybox();
	m_WorldStructure.UniformViewMatrix();
	uint32_t ch = Gd::g_SelectedChunk.load();
	//Setting selected block index, which will be used only by the owning chunk
	Chunk::s_InternalSelectedBlock = Gd::g_SelectedBlock.load();	
	//Uniform light space matrix
	m_State.BlockShader()->UniformMat4f(GlCore::g_DepthSpaceMatrix, "light_space");

	//Draw to scene
	for (uint32_t i = 0; i < MC_CHUNK_SIZE; i++)
	{
		//Wait if the vector is being modified
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (chunk->IsChunkRenderable() && chunk->IsChunkVisible())
			chunk->ForwardRenderableData(position_buffer.get(), texture_buffer.get(), count, false, ch == i);
	}

	//Render any leftover data
	GlCore::DispatchBlockRendering(position_buffer.get(), texture_buffer.get(), count);

	//Draw water layers(normal instanced rendering for the water layer)
	glEnable(GL_BLEND);
	m_State.WaterShader()->Use();
	m_State.WaterVM()->BindVertexArray();
	for (auto vec : m_DrawableWaterLayers)
	{
		//Should never be empty
		m_State.WaterVM()->EditInstance(0, vec->data(), vec->size() * sizeof(glm::vec3), 0);
		GlCore::Renderer::RenderInstanced(vec->size());
	}
	glDisable(GL_BLEND);
	GlCore::g_Drawcalls = 0;
	//Remove pointers from vector, no deletion involved
	m_DrawableWaterLayers.clear();
	//Drawing crossaim
	m_WorldStructure.RenderCrossaim();
}

void World::UpdateScene()
{
	//Chunk dynamic spawning
	auto& camera_position = m_State.GameCamera().GetPosition();
	glm::vec2 camera_2d(camera_position.x, camera_position.z);
	
	if (!GlCore::g_SerializationRunning)
	{

		for (uint32_t i = 0; i < m_SpawnableChunks.size(); i++)
		{
			const glm::vec3& vec = m_SpawnableChunks[i];
			glm::vec2 vec_2d(vec.x, vec.z);
			//Check if a new chunk can be generated
			if (glm::length(vec_2d - camera_2d) < Gd::g_ChunkSpawningDistance)
			{
				glm::vec3 origin_chunk_pos = vec - Chunk::GetHalfWayVector();
				glm::vec2 chunk_pos = { origin_chunk_pos.x, origin_chunk_pos.z };

				if constexpr (GlCore::g_MultithreadedRendering)
				{
					if (m_Chunks.size() == m_Chunks.capacity())
					{
						MC_LOCK(m_Chunks);
						m_Chunks.reserve(m_Chunks.capacity() + m_Chunks.capacity() / 2);
						MC_UNLOCK(m_Chunks);
					}
				}

				std::shared_ptr<Chunk> this_chunk = std::make_shared<Chunk>(*this, chunk_pos);
				m_Chunks.emplace_back(this_chunk);
				m_SafeChunkSize++;
				HandleSectionData();

				this_chunk->InitGlobalNorms();

				//Removing previously visible normals from old chunks && update local chunks
				//The m_Chunk.size() - 1 index is the effective index of the newly pushed chunk
				if (auto opt = this_chunk->GetLoadedChunk(Gd::ChunkLocation::PlusX); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::MinusX, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Gd::ChunkLocation::MinusX); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::PlusX, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Gd::ChunkLocation::PlusZ); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::MinusZ, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Gd::ChunkLocation::MinusZ); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Gd::ChunkLocation::PlusZ, this_chunk->Index());
				}

				//Pushing new spawnable vectors
				if (IsPushable(*this_chunk, Gd::ChunkLocation::PlusX, vec + glm::vec3(16.0f, 0.0f, 0.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(16.0f, 0.0f, 0.0f));
				if (IsPushable(*this_chunk, Gd::ChunkLocation::MinusX, vec + glm::vec3(-16.0f, 0.0f, 0.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(-16.0f, 0.0f, 0.0f));
				if (IsPushable(*this_chunk, Gd::ChunkLocation::PlusZ, vec + glm::vec3(0.0f, 0.0f, 16.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, 16.0f));
				if (IsPushable(*this_chunk, Gd::ChunkLocation::MinusZ, vec + glm::vec3(0.0f, 0.0f, -16.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, -16.0f));

				//Remove the just spawned chunk from the spawnable list
				m_SpawnableChunks.erase(m_SpawnableChunks.begin() + i);

				//Order chunk spawning order based on the distance with the player
				static auto spawner_sorter = [&](const glm::vec3& v1, const glm::vec3& v2) {
					return glm::length(camera_position - v1) < glm::length(camera_position - v2);
				};

				//Sort the vector each second, doing this every frame would be pointless
				if (m_SortingTimer.GetElapsedSeconds() > 1.0f)
				{
					std::sort(m_SpawnableChunks.begin(), m_SpawnableChunks.end(), spawner_sorter);
					m_SortingTimer.StartTimer();
				}

				break;
			}
		}

	}

	//Determine selection
	HandleSelection();

	//normal updating
	for (uint32_t i = 0; i < MC_CHUNK_SIZE; i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (!chunk->IsChunkRenderable() || !chunk->IsChunkVisible())
			continue;

		//We update blocks drawing conditions only if we move or if we break blocks
		chunk->UpdateBlocks();
	}

	if constexpr (GlCore::g_MultithreadedRendering)
		if (GlCore::g_SerializationRunning)
			return;

	static int sercount = 0;

	//Serialization (Working but still causing random crashes sometimes)
	for (Gd::SectionData& data : m_SectionsData)
	{
		//Serialization zone
		if (data.loaded && glm::length(camera_2d - data.central_position) > 750.0f)
		{
			data.loaded = false;

			if constexpr (GlCore::g_MultithreadedRendering)
			{
				auto parallel_serialization = [&]()
					{
						std::cout << sercount++ << "\nSerializing:" << m_Chunks.size() << std::endl;
						SerializeSector(data.index);
						std::cout << "Serialized:" << m_Chunks.size() << std::endl;
						GlCore::g_SerializationRunning = false;
					};

				GlCore::g_SerializationRunning = true;
				m_SerializingFut = std::async(std::launch::async, parallel_serialization);
				//let our thread run alone for now
				break;
			}
			else
			{
				SerializeSector(data.index);
				std::cout << "Serialized:" << m_Chunks.size() << std::endl;
			}
		}

		//Deserialization zone
		if (!data.loaded && glm::length(camera_2d - data.central_position) < 650.0f)
		{
			data.loaded = true;

			if constexpr (GlCore::g_MultithreadedRendering)
			{
				auto parallel_serialization = [&]()
				{
					std::cout << sercount++ << "\nDeserializing:" << m_Chunks.size() << std::endl;
					DeserializeSector(data.index);
					std::cout << "Deserialized:" << m_Chunks.size() << std::endl;
					GlCore::g_SerializationRunning = false;
				};

				GlCore::g_SerializationRunning = true;
				m_SerializingFut = std::async(std::launch::async, parallel_serialization);
				break;
			}
			else
			{
				DeserializeSector(data.index);
				std::cout << "Deserialized:" << m_Chunks.size() << std::endl;
			}
		}
	}
}

void World::HandleSelection()
{
	float nearest_selection = INFINITY;
	int32_t involved_chunk = static_cast<uint32_t>(-1);

	auto& window = m_State.GameWindow();

	bool left_click = window.IsMouseEvent({ GLFW_MOUSE_BUTTON_1, GLFW_PRESS });
	//One block at a time
	bool right_click = window.IsKeyPressed(GLFW_MOUSE_BUTTON_2);

	for (uint32_t i = 0; i < MC_CHUNK_SIZE; i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (!chunk->IsChunkRenderable() || !chunk->IsChunkVisible())
			continue;

		float current_selection = chunk->BlockCollisionLogic(left_click, right_click);

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
		Gd::g_SelectedBlock = m_Chunks[involved_chunk]->LastSelectedBlock();
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

void World::PushWaterLayer(std::shared_ptr<std::vector<glm::vec3>> vec)
{
	m_DrawableWaterLayers.push_back(vec);
}

std::optional<uint32_t> World::IsChunk(const Chunk& chunk, const Gd::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.GetChunkOrigin();
	static auto find_alg = [&](const glm::vec2& pos) ->std::optional<uint32_t>
	{
		for (uint32_t i = 0; i < m_Chunks.size(); i++)
		{
			if (m_Chunks[i]->GetChunkOrigin() == pos)
				return m_Chunks[i]->Index();
		}

		return std::nullopt;
	};

	switch (cl)
	{
	case Gd::ChunkLocation::PlusX:
		return find_alg(origin + glm::vec2(16.0f, 0.0f));
	case Gd::ChunkLocation::MinusX:
		return find_alg(origin - glm::vec2(16.0f, 0.0f));
	case Gd::ChunkLocation::PlusZ:
		return find_alg(origin + glm::vec2(0.0f, 16.0f));
	case Gd::ChunkLocation::MinusZ:
		return find_alg(origin - glm::vec2(0.0f, 16.0f));
	}

	return std::nullopt;
}

Chunk& World::GetChunk(uint32_t index)
{
	auto iter = std::find_if(m_Chunks.begin(), m_Chunks.end(), 
		[index](std::shared_ptr<Chunk> c) {return c->Index() == index; });

	if (iter == m_Chunks.end())
		throw std::runtime_error("Chunk element with this index not found");

	return **iter;
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
	std::vector<std::shared_ptr<Chunk>>::iterator iter{};
	if constexpr (GlCore::g_MultithreadedRendering)
	{
		MC_LOCK(m_Chunks);
		//Rearrange all the elements so that the ones that need to be serialized are at the end
		iter = std::partition(m_Chunks.begin(), m_Chunks.end(), 
			[&](const std::shared_ptr<Chunk>& ch) {return ch->SectorIndex() != index; });
		//Determine safe iteration range for the renderer thread
		m_SafeChunkSize = static_cast<uint32_t>(iter - m_Chunks.begin());
		MC_UNLOCK(m_Chunks);
	}

	//Nothing to serialize
	if (m_Chunks.size() == m_SafeChunkSize)
		return;

	//Number of chunks at the beginning (leave blank for now)
	sz.Serialize<uint32_t>(0);
	for(uint32_t i = 0; i < m_Chunks.size() - m_SafeChunkSize; i++)
	{
		Chunk& chunk = *m_Chunks[m_SafeChunkSize + i];
		chunk.Serialize(sz);
		serialized_chunks++;
	}
	
	MC_LOCK(m_Chunks);
	m_Chunks.erase(iter, m_Chunks.end());
	MC_UNLOCK(m_Chunks);

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
		MC_LOCK(m_Chunks);
		uint32_t deser_size = 0;
		sz% deser_size;
		m_Chunks.reserve(m_Chunks.size() + deser_size);
		MC_UNLOCK(m_Chunks);
	}
	else
	{
		//Not used
		uint32_t deser_size = 0;
		sz% deser_size;
	}

	while (!sz.Eof())
	{
		auto new_chunk = std::make_shared<Chunk>(*this, sz, index);
		m_Chunks.push_back(new_chunk);
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

