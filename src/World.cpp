#include "World.h"
#include "Renderer.h"
#include <atomic>

//Initializing a single block for now
World::World()
	: m_State(GlCore::State::GetState()), m_LastPos(0.0f)
{
	using namespace Defs;

	//Init opengl resources
	GlCore::LoadResources();

	//Init world seed
	m_WorldSeed.seed_value = 1;
	PerlNoise::InitSeedMap(m_WorldSeed);

	for (int32_t i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (int32_t j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement)
			m_Chunks.emplace_back(*this, glm::vec2(float(i), float(j)));

	HandleSectionData();
	//Set safe iterable size for the rendering thread(set in all cases by default	

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
	GlCore::UniformProjMatrix();

	//Load water texture
	auto& textures = GlCore::GameTextures();

	uint32_t water_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureWater);
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

	//Map the shader buffers so we can use them to write data
	glm::vec3* block_positions = static_cast<glm::vec3*>(block_vm->InstancedAttributePointer(0));
	glm::vec3* depth_positions = static_cast<glm::vec3*>(depth_vm->InstancedAttributePointer(0));
	glm::vec3* water_positions = static_cast<glm::vec3*>(m_State.WaterVM()->InstancedAttributePointer(0));
	uint32_t* block_texindices = static_cast<uint32_t*>(block_vm->InstancedAttributePointer(1));

	uint32_t count = 0;

	//If at least one of this conditions are verified, we need to update the shadow texture
	if (Defs::g_EnvironmentChange || glm::length(m_LastPos - camera.GetPosition()) > 10.0f)
	{
		//Reset state
		Defs::g_EnvironmentChange = false;
		m_LastPos = camera.GetPosition();

		glViewport(0, 0, GlCore::g_DepthMapWidth, GlCore::g_DepthMapHeight);
		m_State.DepthFramebuffer()->Bind();
		Window::ClearScreen(GL_DEPTH_BUFFER_BIT);

		//Update depth framebuffer
		GlCore::UpdateShadowFramebuffer();

		for (uint32_t i = 0; i < m_Chunks.size(); i++)
		{
			//Interrupt for a moment if m_Chunks is being resized by the logic thread
			std::shared_ptr<Chunk> chunk = m_Chunks[i];
			//When we do this, that means that the resource and the ones after this are no longer available
			//and there is thus no need to iterate over them
			if (chunk == nullptr)
				break;

			if (chunk->IsChunkRenderable() && chunk->IsChunkVisibleByShadow())
				chunk->ForwardRenderableData(depth_positions, block_texindices, count, true);
		}

		//Render any leftover data
		GlCore::DispatchDepthRendering(depth_positions, count);

		uint32_t depth_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureDepth);
		m_State.DepthFramebuffer()->BindFrameTexture(depth_binding);
		m_State.BlockShader()->Uniform1i(depth_binding, "texture_depth");

		glViewport(0, 0, window.Width(), window.Height());
		FrameBuffer::BindDefault();
	}

	//Render skybox
	Window::ClearScreen(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GlCore::RenderSkybox();
	GlCore::UniformViewMatrix();
	uint32_t ch = Defs::g_SelectedChunk.load();
	//Setting selected block index, which will be used only by the owning chunk
	Chunk::s_InternalSelectedBlock = Defs::g_SelectedBlock.load();	
	//Uniform light space matrix
	m_State.BlockShader()->UniformMat4f(GlCore::g_DepthSpaceMatrix, "light_space");

	//Draw to scene
	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		//Wait if the vector is being modified
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (chunk == nullptr)
			break;
		
		if (chunk->IsChunkRenderable() && chunk->IsChunkVisible())
			chunk->ForwardRenderableData(block_positions, block_texindices, count, false, ch == i);
	}

	//Render any leftover data
	GlCore::DispatchBlockRendering(block_positions, block_texindices, count);

	count = 0;
	//Draw water layers(normal instanced rendering for the water layer)
	glEnable(GL_BLEND);
	m_State.WaterShader()->Use();
	m_State.WaterVM()->BindVertexArray();
	for (auto vec : m_DrawableWaterLayers)
	{
		std::memcpy(water_positions + count, vec->data(), vec->size() * sizeof(glm::vec3));
		count += vec->size();
	}
	//Render water layers
	m_State.WaterVM()->UnmapAttributePointer(0);
	GlCore::Renderer::RenderInstanced(count);

	glDisable(GL_BLEND);
	GlCore::RenderCrossaim();
	
	m_DrawableWaterLayers.clear();
	GlCore::g_Drawcalls = 0;
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
			if (glm::length(vec_2d - camera_2d) < Defs::g_ChunkSpawningDistance)
			{
				glm::vec3 origin_chunk_pos = vec - Chunk::GetHalfWayVector();
				glm::vec2 chunk_pos = { origin_chunk_pos.x, origin_chunk_pos.z };

				//Generate new chunk
				std::shared_ptr<Chunk> this_chunk = std::make_shared<Chunk>(*this, chunk_pos);
				m_Chunks.push_back(this_chunk);
				HandleSectionData();

				this_chunk->InitGlobalNorms();

				//Removing previously visible normals from old chunks && update local chunks
				//The m_Chunk.size() - 1 index is the effective index of the newly pushed chunk
				if (auto opt = this_chunk->GetLoadedChunk(Defs::ChunkLocation::PlusX); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Defs::ChunkLocation::MinusX, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Defs::ChunkLocation::MinusX); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Defs::ChunkLocation::PlusX, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Defs::ChunkLocation::PlusZ); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Defs::ChunkLocation::MinusZ, this_chunk->Index());
				}
				if (auto opt = this_chunk->GetLoadedChunk(Defs::ChunkLocation::MinusZ); opt.has_value())
				{
					auto& neighbor_chunk = GetChunk(opt.value());
					neighbor_chunk.SetLoadedChunk(Defs::ChunkLocation::PlusZ, this_chunk->Index());
				}

				//Pushing new spawnable vectors
				if (IsPushable(*this_chunk, Defs::ChunkLocation::PlusX, vec + glm::vec3(16.0f, 0.0f, 0.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(16.0f, 0.0f, 0.0f));
				if (IsPushable(*this_chunk, Defs::ChunkLocation::MinusX, vec + glm::vec3(-16.0f, 0.0f, 0.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(-16.0f, 0.0f, 0.0f));
				if (IsPushable(*this_chunk, Defs::ChunkLocation::PlusZ, vec + glm::vec3(0.0f, 0.0f, 16.0f)))
					m_SpawnableChunks.push_back(vec + glm::vec3(0.0f, 0.0f, 16.0f));
				if (IsPushable(*this_chunk, Defs::ChunkLocation::MinusZ, vec + glm::vec3(0.0f, 0.0f, -16.0f)))
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
	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (m_Chunks[i] == nullptr)
			break;

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
	for (Defs::SectionData& data : m_SectionsData)
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

	//One block at a time
	bool left_click = window.IsKeyPressed(GLFW_MOUSE_BUTTON_1);
	bool right_click = window.IsKeyPressed(GLFW_MOUSE_BUTTON_2);

	for (uint32_t i = 0; i < m_Chunks.size(); i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (chunk == nullptr)
			break;

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
	Defs::g_SelectedChunk = involved_chunk;
	if (involved_chunk != static_cast<uint32_t>(-1))
	{
		Defs::g_SelectedBlock = m_Chunks[involved_chunk]->LastSelectedBlock();
	}
}

void World::HandleSectionData()
{
	//Load pushed sections
	for (auto& obj : Defs::g_PushedSections)
	{
		//Checking that it was now pushed yet
		if (std::find_if(m_SectionsData.begin(), m_SectionsData.end(),
			[&](const auto& elem) {return elem.index == obj; }) == m_SectionsData.end())
		{
			Defs::SectionData sd{ obj, SectionCentralPosFrom(obj), true };
			m_SectionsData.push_back(sd);
		}
	}

	Defs::g_PushedSections.clear();
}

void World::PushWaterLayer(std::shared_ptr<std::vector<glm::vec3>> vec)
{
	m_DrawableWaterLayers.push_back(vec);
}

std::optional<uint32_t> World::IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl)
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
	case Defs::ChunkLocation::PlusX:
		return find_alg(origin + glm::vec2(16.0f, 0.0f));
	case Defs::ChunkLocation::MinusX:
		return find_alg(origin - glm::vec2(16.0f, 0.0f));
	case Defs::ChunkLocation::PlusZ:
		return find_alg(origin + glm::vec2(0.0f, 16.0f));
	case Defs::ChunkLocation::MinusZ:
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

Defs::WorldSeed& World::Seed()
{
	return m_WorldSeed;
}

const Defs::WorldSeed& World::Seed() const
{
	return m_WorldSeed;
}

void World::SerializeSector(uint32_t index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Defs::g_SerializedFileFormat, "wb");
	uint32_t serialized_chunks = 0;

	//(When multithreading) Advertise the render thread m_Chunks 
	//is undergoing some heavy changes
	std::vector<std::shared_ptr<Chunk>>::iterator iter{};
	uint32_t safe_size = 0;
	if constexpr (GlCore::g_MultithreadedRendering)
	{
		MC_LOCK(m_Chunks);
		//Rearrange all the elements so that the ones that need to be serialized are at the end
		iter = std::partition(m_Chunks.begin(), m_Chunks.end(), 
			[&](const std::shared_ptr<Chunk>& ch) {return ch->SectorIndex() != index; });
		//Determine safe iteration range for the renderer thread
		safe_size = static_cast<uint32_t>(iter - m_Chunks.begin());
		MC_UNLOCK(m_Chunks);
	}

	//Nothing to serialize
	if (m_Chunks.size() == safe_size)
		return;

	//Number of chunks at the beginning (leave blank for now)
	sz.Serialize<uint32_t>(0);
	for(uint32_t i = 0; i < m_Chunks.size() - safe_size; i++)
	{
		Chunk& chunk = *m_Chunks[safe_size + i];
		chunk.Serialize(sz);
		serialized_chunks++;
	}
	
	MC_LOCK(m_Chunks);
	m_Chunks.erase(iter, m_Chunks.end());
	MC_UNLOCK(m_Chunks);

	//Write the amount of chunks serialized
	sz.Seek(0);
	sz& serialized_chunks;
}

void World::DeserializeSector(uint32_t index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Defs::g_SerializedFileFormat, "rb");

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
		m_Chunks.emplace_back(*this, sz, index);	
}

bool World::IsPushable(const Chunk& chunk, const Defs::ChunkLocation& cl, const glm::vec3& vec)
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

	glm::vec2 pos = glm::vec2(coords[0], coords[1]) * Defs::g_SectionDimension;
	pos.x += Defs::g_SectionDimension * 0.5f;
	pos.y += Defs::g_SectionDimension * 0.5f;

	return pos;
}

