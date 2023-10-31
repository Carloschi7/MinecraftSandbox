#include "World.h"
#include "Renderer.h"
#include "InventorySystem.h"
#include <atomic>

//Initializing a single block for now
World::World()
	: m_State(*GlCore::pstate), m_LastPos(0.0f)
{
	using namespace Defs;

	//Init opengl resources
	GlCore::LoadResources();

	//16 slots is probably too much space but because it is already pretty small
	//there is no reason to allocate any less
	m_CollisionChunkBuffer = Memory::Allocate(m_State.memory_arena, sizeof(Chunk*) * 16);
	const u16 max_removable_buffers = glm::pow(static_cast<u16>(Defs::g_SectionDimension) / Chunk::s_ChunkWidthAndHeight, 2);
	m_RemovableChunkBuffer = Memory::Allocate(m_State.memory_arena, sizeof(VAddr) * max_removable_buffers);

	//Determine the relative space in which chunks are going to generate their foliage
	for (s32 x = -2.0f; x <= 2.0f; x++) {
		for (s32 y = -1.0f; y <= 3.0f; y++) {
			for (s32 z = -2.0f; z <= 2.0f; z++) {
				relative_leaves_positions.emplace_back((f32)x, (f32)y, (f32)z);
			}
		}
	}

	//Pop the ones in the trunk's way
	for (f32 y = -1.0f; y <= 1.0f; y++) {
		relative_leaves_positions.erase(std::find(relative_leaves_positions.begin(), relative_leaves_positions.end(),
			glm::vec3(0.0f, (f32)y, 0.0f)));
	}

	//Init world seed
	m_WorldSeed.seed_value = 1;
	PerlNoise::InitSeedMap(m_WorldSeed);

	for (s32 i = g_SpawnerBegin; i < g_SpawnerEnd; i += g_SpawnerIncrement)
		for (s32 j = g_SpawnerBegin; j < g_SpawnerEnd; j += g_SpawnerIncrement) 
			m_Chunks.push_back(Memory::New<Chunk>(m_State.memory_arena, *this, glm::vec2(f32(i), f32(j))));

	HandleSectionData();

	auto add_spawnable_chunk = [&](const glm::vec3& pos)
	{
		glm::vec3 res = pos + Chunk::GetHalfWayVector();
		m_SpawnableChunks.emplace_back(res);
	};

	//Init spawnable chunks
	for (auto& chunk_ptr : m_Chunks)
	{
		Chunk& chunk = *Memory::Get<Chunk>(m_State.memory_arena, chunk_ptr);
		const glm::vec2& chunk_pos = chunk.ChunkOrigin2D();

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
	u32 water_binding = static_cast<u32>(Defs::TextureBinding::TextureWater);
	m_State.game_textures[water_binding].Bind(water_binding);
	m_State.water_shader->Uniform1i(water_binding, "texture_water");
}

World::~World()
{
	//Free indipendent memory patches
	Memory::Free(m_State.memory_arena, m_RemovableChunkBuffer);
	Memory::Free(m_State.memory_arena, m_CollisionChunkBuffer);

	for (auto chunk_addr : m_Chunks)
		Memory::Delete<Chunk>(m_State.memory_arena, chunk_addr);
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

	u32 count = 0, water_layer_count = 0;

	//If at least one of this conditions are verified, we need to update the shadow texture
	if (Defs::g_EnvironmentChange || glm::length(m_LastPos - camera_position) > 20.0f)
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
			Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
			//Chunk has probably been serialized (should mostly never happen)
			if (!chunk)
				break;

			if (chunk->IsChunkRenderable(camera_position))
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
		Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
		if (!chunk)
			break;

		if (chunk->IsChunkRenderable(camera_position) && chunk->IsChunkVisible(camera_position, camera_direction)) {
			chunk->ForwardRenderableData(block_positions, block_texindices, count, false, ch == i);
			chunk->RenderDrops();

			//Add possible water positions
			chunk->AddWaterLayerIfPresent(water_positions, water_layer_count);
		}
	}

	//Render any leftover data
	GlCore::DispatchBlockRendering(block_positions, block_texindices, count);

	//Draw water layers(normal instanced rendering for the water layer)
	glEnable(GL_BLEND);
	m_State.water_shader->Use();
	m_State.water_vm->BindVertexArray();
	//Render water layers which have been set by AddWaterLayerIfPresent
	m_State.water_vm->UnmapAttributePointer(0);
	GlCore::Renderer::RenderInstanced(water_layer_count);

	glDisable(GL_BLEND);
	GlCore::RenderCrossaim();

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
				VAddr chunk_addr = Memory::New<Chunk>(m_State.memory_arena, *this, chunk_pos);
				m_Chunks.push_back(chunk_addr);
				//Memory::LockRegion(chunk_addr);
				Chunk* this_chunk = Memory::Get<Chunk>(m_State.memory_arena, chunk_addr);
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

				//Memory::UnlockRegion(chunk_addr);
				break;
			}
		}

	}

	//Determine selection
	HandleSelection(inventory, camera_position, camera_direction);

	//normal updating & player collision
	for (u32 i = 0; i < m_Chunks.size(); i++)
	{
		Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
		if (!chunk)
			break;
		if (!chunk->IsChunkRenderable(camera_position) || !chunk->IsChunkVisible(camera_position, camera_direction))
			continue;

		//We update blocks drawing conditions only if we move or if we break blocks
		chunk->UpdateBlocks(inventory, elapsed_time);
	}

	if constexpr (GlCore::g_MultithreadedRendering)
		if (GlCore::g_SerializationRunning)
			return;

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
						MC_LOG("() => Serializing: {}\n", m_Chunks.size());
						SerializeSector(data.index);
						MC_LOG("Serialized: {}\n", m_Chunks.size());
						GlCore::g_SerializationRunning = false;
					};

				GlCore::g_SerializationRunning = true;
				m_SerializingFut = std::async(std::launch::async, parallel_serialization);
				//let our thread run alone for now
				break;
			}
			else
			{
				MC_LOG("() => Serializing: {}\n", m_Chunks.size());
				SerializeSector(data.index);
				MC_LOG("Serialized: {}\n", m_Chunks.size());
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
					MC_LOG("() => Deserializing: {}", m_Chunks.size());
					DeserializeSector(data.index);
					MC_LOG("Deserialized: {}", m_Chunks.size());
					GlCore::g_SerializationRunning = false;
				};

				GlCore::g_SerializationRunning = true;
				m_SerializingFut = std::async(std::launch::async, parallel_serialization);
				break;
			}
			else
			{
				MC_LOG("() => Deserializing: {}", m_Chunks.size());
				DeserializeSector(data.index);
				MC_LOG("Deserialized: {}", m_Chunks.size());
			}
		}
	}
}

void World::HandleSelection(Inventory& inventory, const glm::vec3& camera_position, const glm::vec3& camera_direction)
{
	f32 nearest_distance = INFINITY;
	Defs::HitDirection hit = Defs::HitDirection::None;
	s32 involved_chunk = static_cast<u32>(-1);

	auto window = m_State.game_window;

	//One block at a time
	bool left_click = window->IsKeyPressed(GLFW_MOUSE_BUTTON_1);
	bool right_click = window->IsKeyPressed(GLFW_MOUSE_BUTTON_2);

	for (u32 i = 0; i < m_Chunks.size(); i++)
	{
		Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
		if (!chunk)
			break;
		if (!chunk->IsChunkRenderable(camera_position) || !chunk->IsChunkVisible(camera_position, camera_direction))
			continue;

		const auto [hit_distance, hit_direction] = chunk->RayCollisionLogic(camera_position, camera_direction);
		if (hit_distance < nearest_distance)
		{
			nearest_distance = hit_distance;
			hit = hit_direction;
			involved_chunk = i;
		}
	}

	//Setting from which chunk the selectedd block comes from
	Defs::g_SelectedChunk = involved_chunk;
	if (involved_chunk != static_cast<u32>(-1))
	{
		Chunk* local_chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[involved_chunk]);
		Defs::g_SelectedBlock = local_chunk->LastSelectedBlock();
		if (Defs::g_ViewMode != Defs::ViewMode::Inventory && (left_click || right_click)) 
		{
			u32 selected_block = Defs::g_SelectedBlock;
			auto& blocks = local_chunk->Blocks();
			if (left_click && selected_block != static_cast<u32>(-1))
			{
				const glm::vec3 position = local_chunk->ToWorld(blocks[selected_block].position);
				const Defs::BlockType type = blocks[selected_block].Type();

				local_chunk->AddNewExposedNormals(position);
				blocks.erase(blocks.begin() + selected_block);

				//Push drop
				local_chunk->PushDrop(position, type);
			}

			//Push a new block
			if (right_click && selected_block != static_cast<u32>(-1))
			{
				//if the selected block isn't -1 that means selection is not NONE
				Defs::BlockType bt = Defs::g_InventorySelectedBlock;
				auto& block = blocks[selected_block];

				//Remove one unit from the selection
				std::optional<InventoryEntry>& entry = inventory.HoveredFromSelector();

				//No block selected, no block inserted
				if (!entry.has_value())
					return;

				entry.value().block_count--;
				inventory.ClearUsedSlots();
				bool block_added_to_side_chunk = false;
				switch (hit)
				{
				case Defs::HitDirection::PosX: {

					if (block.position.x == Chunk::s_ChunkWidthAndHeight - 1) {
						std::optional<u32> adjacent_chunk_index = local_chunk->GetLoadedChunk(Defs::ChunkLocation::PlusX);
						MC_ASSERT(adjacent_chunk_index.has_value());
						auto& adjacent_chunk = GetChunk(adjacent_chunk_index.value());
						auto& emplaced_block = adjacent_chunk.Blocks().emplace_back(glm::u8vec3{ 0, block.position.y, block.position.z }, bt);
						adjacent_chunk.AddFreshNormals(emplaced_block);
						block_added_to_side_chunk = true;
					}
					else {
						blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_PosX), bt);
					}

				} break;
				case Defs::HitDirection::NegX: {

					if (block.position.x == 0) {
						//The chunk is adjacent to the chunk we are in, should definitely be loaded
						std::optional<u32> adjacent_chunk_index = local_chunk->GetLoadedChunk(Defs::ChunkLocation::MinusX);
						MC_ASSERT(adjacent_chunk_index.has_value());
						auto& adjacent_chunk = GetChunk(adjacent_chunk_index.value());
						auto& emplaced_block = adjacent_chunk.Blocks().emplace_back(glm::u8vec3{ Chunk::s_ChunkWidthAndHeight - 1, block.position.y, block.position.z}, bt);
						adjacent_chunk.AddFreshNormals(emplaced_block);
						block_added_to_side_chunk = true;
					}
					else {
						blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_NegX), bt);
					}
				} break;
				case Defs::HitDirection::PosY:
					blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_PosY), bt);
					break;
				case Defs::HitDirection::NegY:
					blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_NegY), bt);
					break;
				case Defs::HitDirection::PosZ: {
					
					if (block.position.z == Chunk::s_ChunkWidthAndHeight - 1) {
						std::optional<u32> adjacent_chunk_index = local_chunk->GetLoadedChunk(Defs::ChunkLocation::PlusZ);
						MC_ASSERT(adjacent_chunk_index.has_value());
						auto& adjacent_chunk = GetChunk(adjacent_chunk_index.value());
						auto& emplaced_block = adjacent_chunk.Blocks().emplace_back(glm::u8vec3{ block.position.x, block.position.y, 0 }, bt);
						adjacent_chunk.AddFreshNormals(emplaced_block);
						block_added_to_side_chunk = true;
					}
					else {
						blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_PosZ), bt);
					}
					
				} break;
				case Defs::HitDirection::NegZ: {

					if (block.position.z == 0) {
						//The chunk is adjacent to the chunk we are in, should definitely be loaded
						std::optional<u32> adjacent_chunk_index = local_chunk->GetLoadedChunk(Defs::ChunkLocation::MinusZ);
						MC_ASSERT(adjacent_chunk_index.has_value());
						auto& adjacent_chunk = GetChunk(adjacent_chunk_index.value());
						auto& emplaced_block = adjacent_chunk.Blocks().emplace_back(glm::u8vec3{ block.position.x, block.position.y, Chunk::s_ChunkWidthAndHeight - 1 }, bt);
						adjacent_chunk.AddFreshNormals(emplaced_block);
						block_added_to_side_chunk = true;
					}
					else {
						blocks.emplace_back(block.position + static_cast<glm::u8vec3>(GlCore::g_NegZ), bt);
					}
				} break;

				case Defs::HitDirection::None:
					MC_ASSERT(false);
					break;
				}

				if(!block_added_to_side_chunk)
					local_chunk->AddFreshNormals(blocks.back());
			}
			//Signal block has been destroyed
			Defs::g_EnvironmentChange = true;
		}
	}
}

void World::CheckPlayerCollision(const glm::vec3& position)
{
	u16 count = 0;
	Chunk** chunk_buffer = Memory::Get<Chunk*>(m_State.memory_arena, m_CollisionChunkBuffer);

	for (u32 i = 0; i < m_Chunks.size(); i++) {
		Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
		if (!chunk)
			break;
		if (glm::length(glm::vec2(position.x, position.z) - glm::vec2(chunk->ChunkCenter().x, chunk->ChunkCenter().z)) < 12.0f)
			chunk_buffer[count++] = chunk;
	}

	Camera* cam = m_State.camera;
	for(u16 i = 0; i < count; i++)
		chunk_buffer[i]->BlockCollisionLogic(cam->position);
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

std::optional<u32> World::IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl)
{
	const glm::vec2& origin = chunk.ChunkOrigin2D();
	static auto find_alg = [&](const glm::vec2& pos) ->std::optional<u32>
	{
		for (u32 i = 0; i < m_Chunks.size(); i++)
		{
			Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, m_Chunks[i]);
			if (!chunk)
				break;
			if (chunk->ChunkOrigin2D() == pos)
				return chunk->Index();
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
		[this, index](VAddr addr) {return Memory::Get<Chunk>(m_State.memory_arena, addr)->Index() == index; });

	MC_ASSERT(iter != m_Chunks.end(), "the provided variable index should be valid");

	return *Memory::Get<Chunk>(m_State.memory_arena, *iter);
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
	u16 count = 0;
	VAddr* removable_chunks = Memory::Get<VAddr>(m_State.memory_arena, m_RemovableChunkBuffer);
	if constexpr (GlCore::g_MultithreadedRendering)
	{
		for (u32 i = 0; i < m_Chunks.size(); i++)
			Memory::LockRegion(m_State.memory_arena, m_Chunks[i]);
		//Rearrange all the elements so that the ones that need to be serialized are at the end
		auto iter = std::partition(m_Chunks.begin(), m_Chunks.end(), [this, index](VAddr addr) 
			{
				Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, addr);
				return chunk->SectorIndex() != index; 
			});
		//Determine safe iteration range for the renderer thread
		for (auto sub_iter = iter; sub_iter != m_Chunks.end(); ++sub_iter) {
			removable_chunks[count++] = *sub_iter;
		}
		
		m_Chunks.erase(iter, m_Chunks.end());
		for (u32 i = 0; i < m_Chunks.size(); i++)
			Memory::UnlockRegion(m_State.memory_arena, m_Chunks[i]);
	}

	//Nothing to serialize
	if (count == 0)
		return;

	//Number of chunks at the beginning (leave blank for now)
	sz.Serialize<u32>(0);
	for(u16 i = 0; i < count; i++)
	{
		Chunk* chunk = Memory::Get<Chunk>(m_State.memory_arena, removable_chunks[i]);
		chunk->Serialize(sz);
		Memory::UnlockRegion(m_State.memory_arena, removable_chunks[i]);
		Memory::Delete<Chunk>(m_State.memory_arena, removable_chunks[i]);
		serialized_chunks++;
	}

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
		for (u32 i = 0; i < m_Chunks.size(); i++)
			Memory::LockRegion(m_State.memory_arena, m_Chunks[i]);
		u32 deser_size = 0;
		sz% deser_size;
		m_Chunks.reserve(m_Chunks.size() + deser_size);
		for (u32 i = 0; i < m_Chunks.size(); i++)
			Memory::UnlockRegion(m_State.memory_arena, m_Chunks[i]);
	}
	else
	{
		//Not used
		u32 deser_size = 0;
		sz% deser_size;
	}

	while (!sz.Eof())	
		m_Chunks.emplace_back(Memory::New<Chunk>(m_State.memory_arena, *this, sz, index));
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

