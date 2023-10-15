#include "World.h"
#include "Renderer.h"
#include "InventorySystem.h"
#include <atomic>


//Initializing a single block for now
World::World()
	: m_State(GlCore::State::GlobalInstance()), m_LastPos(0.0f)
{
	using namespace Defs;

	//Init opengl resources
	GlCore::LoadResources();

	//Init world seed
	m_WorldSeed.seed_value = 1;
	PerlNoise::InitSeedMap(m_WorldSeed);

	for (s32 i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (s32 j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement)
			m_Chunks.emplace_back(*this, glm::vec2(f32(i), f32(j)));

	HandleSectionData();

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

	u32 water_binding = static_cast<u32>(Defs::TextureBinding::TextureWater);
	textures[water_binding].Bind(water_binding);
	m_State.water_shader->Uniform1i(water_binding, "texture_water");
}

World::~World()
{
}

void World::Render(const glm::vec3& camera_position, const glm::vec3& camera_direction)
{
	//Draw to depth framebuffer
	auto block_vm = m_State.block_vm;
	auto depth_vm = m_State.depth_vm;

	//Map the shader buffers so we can use them to write data
	glm::vec3* block_positions = static_cast<glm::vec3*>(block_vm->InstancedAttributePointer(0));
	glm::vec3* depth_positions = static_cast<glm::vec3*>(depth_vm->InstancedAttributePointer(0));
	glm::vec3* water_positions = static_cast<glm::vec3*>(m_State.water_vm->InstancedAttributePointer(0));
	u32* block_texindices = static_cast<u32*>(block_vm->InstancedAttributePointer(1));

	u32 count = 0;

	//If at least one of this conditions are verified, we need to update the shadow texture
	if (Defs::g_EnvironmentChange || glm::length(m_LastPos - camera_position) > 10.0f)
	{
		//Reset state
		Defs::g_EnvironmentChange = false;
		m_LastPos = camera_position;

		glViewport(0, 0, GlCore::g_DepthMapWidth, GlCore::g_DepthMapHeight);
		m_State.shadow_framebuffer->Bind();
		Window::ClearScreen(GL_DEPTH_BUFFER_BIT);

		//Update depth framebuffer
		GlCore::UpdateShadowFramebuffer();

		for (u32 i = 0; i < m_Chunks.size(); i++)
		{
			//Interrupt for a moment if m_Chunks is being resized by the logic thread
			std::shared_ptr<Chunk> chunk = m_Chunks[i];
			//When we do this, that means that the resource and the ones after this are no longer available
			//and there is thus no need to iterate over them
			if (chunk == nullptr)
				break;

			if (chunk->IsChunkRenderable(camera_position) && chunk->IsChunkVisibleByShadow(camera_position, camera_direction))
				chunk->ForwardRenderableData(depth_positions, block_texindices, count, true);
		}

		//Render any leftover data
		GlCore::DispatchDepthRendering(depth_positions, count);

		u32 depth_binding = static_cast<u32>(Defs::TextureBinding::TextureDepth);
		m_State.shadow_framebuffer->BindFrameTexture(depth_binding);
		m_State.block_shader->Uniform1i(depth_binding, "texture_depth");

		glViewport(0, 0, Defs::g_ScreenWidth, Defs::g_ScreenHeight);
		FrameBuffer::BindDefault();
	}

	//Render skybox
	Window::ClearScreen(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GlCore::RenderSkybox();
	//Init matrices
	GlCore::UniformViewMatrix();
	u32 ch = Defs::g_SelectedChunk.load();
	//Setting selected block index, which will be used only by the owning chunk
	Chunk::s_InternalSelectedBlock = Defs::g_SelectedBlock.load();	
	//Uniform light space matrix
	m_State.block_shader->UniformMat4f(GlCore::g_DepthSpaceMatrix, "light_space");

	//Draw to scene
	for (u32 i = 0; i < m_Chunks.size(); i++)
	{
		//Wait if the vector is being modified
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (chunk == nullptr)
			break;
		
		if (chunk->IsChunkRenderable(camera_position) && chunk->IsChunkVisible(camera_position, camera_direction)) {
			chunk->ForwardRenderableData(block_positions, block_texindices, count, false, ch == i);
			chunk->RenderDrops();
		}
	}

	//Render any leftover data
	GlCore::DispatchBlockRendering(block_positions, block_texindices, count);

	count = 0;
	//Draw water layers(normal instanced rendering for the water layer)
	glEnable(GL_BLEND);
	m_State.water_shader->Use();
	m_State.water_vm->BindVertexArray();
	for (auto vec : m_DrawableWaterLayers)
	{
		std::memcpy(water_positions + count, vec->data(), vec->size() * sizeof(glm::vec3));
		count += vec->size();
	}
	//Render water layers
	m_State.water_vm->UnmapAttributePointer(0);
	GlCore::Renderer::RenderInstanced(count);

	glDisable(GL_BLEND);
	GlCore::RenderCrossaim();

	m_DrawableWaterLayers.clear();
	GlCore::g_Drawcalls = 0;
}

void World::UpdateScene(Inventory& inventory, f32 elapsed_time)
{
	//Chunk dynamic spawning
	auto& camera_position = m_State.camera->GetPosition();
	auto& camera_direction = m_State.camera->GetFront();
	glm::vec2 camera_2d(camera_position.x, camera_position.z);
	
	if (!GlCore::g_SerializationRunning)
	{

		for (u32 i = 0; i < m_SpawnableChunks.size(); i++)
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
	HandleSelection(inventory, camera_position, camera_direction);
	Physics::ProcessPlayerAxisMovement(elapsed_time);

	//normal updating & player collision
	for (u32 i = 0; i < m_Chunks.size(); i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (m_Chunks[i] == nullptr)
			break;

		if (!chunk->IsChunkRenderable(camera_position) || !chunk->IsChunkVisible(camera_position, camera_direction))
			continue;

		//We update blocks drawing conditions only if we move or if we break blocks
		chunk->UpdateBlocks(inventory, elapsed_time);
		//Check if the player collides with blocks himself
		Camera* cam = m_State.camera;
		chunk->BlockCollisionLogic(cam->position);
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

void World::HandleSelection(Inventory& inventory, const glm::vec3& camera_position, const glm::vec3& camera_direction)
{
	f32 nearest_selection = INFINITY;
	s32 involved_chunk = static_cast<u32>(-1);

	auto window = m_State.game_window;

	//One block at a time
	bool left_click = window->IsMouseEvent({ GLFW_MOUSE_BUTTON_1, GLFW_PRESS });
	bool right_click = window->IsKeyPressed(GLFW_MOUSE_BUTTON_2);

	for (u32 i = 0; i < m_Chunks.size(); i++)
	{
		std::shared_ptr<Chunk> chunk = m_Chunks[i];
		if (chunk == nullptr)
			break;

		if (!chunk->IsChunkRenderable(camera_position) || !chunk->IsChunkVisible(camera_position, camera_direction))
			continue;

		f32 current_selection = chunk->RayCollisionLogic(inventory, left_click, right_click);

		if (current_selection < nearest_selection)
		{
			nearest_selection = current_selection;
			involved_chunk = i;
		}
	}

	//Setting from which chunk the selectedd block comes from
	Defs::g_SelectedChunk = involved_chunk;
	if (involved_chunk != static_cast<u32>(-1))
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

std::optional<u32> World::IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.GetChunkOrigin();
	static auto find_alg = [&](const glm::vec2& pos) ->std::optional<u32>
	{
		for (u32 i = 0; i < m_Chunks.size(); i++)
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

Chunk& World::GetChunk(u32 index)
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

void World::SerializeSector(u32 index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Defs::g_SerializedFileFormat, "wb");
	u32 serialized_chunks = 0;

	//(When multithreading) Advertise the render thread m_Chunks 
	//is undergoing some heavy changes
	std::vector<std::shared_ptr<Chunk>>::iterator iter{};
	u32 safe_size = 0;
	if constexpr (GlCore::g_MultithreadedRendering)
	{
		MC_LOCK(m_Chunks);
		//Rearrange all the elements so that the ones that need to be serialized are at the end
		iter = std::partition(m_Chunks.begin(), m_Chunks.end(), 
			[&](const std::shared_ptr<Chunk>& ch) {return ch->SectorIndex() != index; });
		//Determine safe iteration range for the renderer thread
		safe_size = static_cast<u32>(iter - m_Chunks.begin());
		MC_UNLOCK(m_Chunks);
	}

	//Nothing to serialize
	if (m_Chunks.size() == safe_size)
		return;

	//Number of chunks at the beginning (leave blank for now)
	sz.Serialize<u32>(0);
	for(u32 i = 0; i < m_Chunks.size() - safe_size; i++)
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

void World::DeserializeSector(u32 index)
{
	//Load sector's serializer
	Utils::Serializer sz("runtime_files/sector_" + std::to_string(index) + Defs::g_SerializedFileFormat, "rb");

	if constexpr (GlCore::g_MultithreadedRendering)
	{
		MC_LOCK(m_Chunks);
		u32 deser_size = 0;
		sz% deser_size;
		m_Chunks.reserve(m_Chunks.size() + deser_size);
		MC_UNLOCK(m_Chunks);
	}
	else
	{
		//Not used
		u32 deser_size = 0;
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

glm::vec2 World::SectionCentralPosFrom(u32 index)
{
	//extract the 2 u16
	s16 coords[2];
	std::memcpy(coords, &index, sizeof(u32));

	glm::vec2 pos = glm::vec2(coords[0], coords[1]) * Defs::g_SectionDimension;
	pos.x += Defs::g_SectionDimension * 0.5f;
	pos.y += Defs::g_SectionDimension * 0.5f;

	return pos;
}

