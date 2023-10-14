#include "Chunk.h"
#include "World.h"
#include "InventorySystem.h"
#include <algorithm>
#include <chrono>

//Half a chunk's diagonal
f32 Chunk::s_DiagonalLenght = 0.0f;
u32 Chunk::s_InternalSelectedBlock = static_cast<u32>(-1);

Chunk::Chunk(World& father, glm::vec2 origin)
	:m_RelativeWorld(father), m_State(GlCore::State::GlobalInstance()),
	m_ChunkOrigin(origin), m_SelectedBlock(static_cast<u32>(-1)),
	m_ChunkCenter(0.0f), m_SectorIndex(0)
{
	//Assigning chunk index
	m_ChunkIndex = Defs::g_ChunkProgIndex++;

	glm::vec3 chunk_origin_3d(m_ChunkOrigin.x, 0.0f, m_ChunkOrigin.y);
	m_ChunkCenter = chunk_origin_3d + GetHalfWayVector();

	if (s_DiagonalLenght == 0.0f)
	{
		f32 lower_diag_squared = 2 * glm::pow(s_ChunkWidthAndHeight, 2);
		s_DiagonalLenght = glm::sqrt(lower_diag_squared + glm::pow(s_ChunkDepth, 2)) * 0.5f;
	}

	//Init water layer position vector
	m_WaterLayerPositions = std::make_shared<std::vector<glm::vec3>>();
	//Chunk tree leaves if present
	std::vector<glm::vec3> leaves_positions = Defs::GenerateRandomFoliage();

	//Insert the y coordinates consecutively to allow the
	//normal insertion algorithm later
	for (s32 i = origin.x; i < origin.x + s_ChunkWidthAndHeight; i++)
	{
		for (s32 k = origin.y; k < origin.y + s_ChunkWidthAndHeight; k++)
		{
			f32 fx = static_cast<f32>(i), fy = static_cast<f32>(k);
			auto perlin_data = Defs::PerlNoise::GetBlockAltitude(fx, fy, m_RelativeWorld.Seed());
			u32 final_height = (s_ChunkDepth - 10) + std::roundf(perlin_data.altitude * 8.0f);

			for (s32 j = 0; j < final_height; j++)
			{
				switch (perlin_data.biome)
				{
				case Defs::Biome::Plains:
					m_LocalBlocks.emplace_back(glm::vec3(i, j, k), (j == final_height - 1) ?
						Defs::BlockType::Grass : Defs::BlockType::Dirt);
					break;
				case Defs::Biome::Desert:
					m_LocalBlocks.emplace_back(glm::vec3(i, j, k), Defs::BlockType::Sand);
					break;
				}
			}

			if (perlin_data.in_water)
			{
				f32 water_level = Defs::WaterRegionLevel(fx, fy, m_RelativeWorld.Seed());
				u32 water_height = (s_ChunkDepth - 10) + std::roundf(water_level * 8.0f) - 1;

				if (water_height >= final_height)
					m_WaterLayerPositions->push_back(glm::vec3(i, water_height, k));

				continue;
			}

			glm::vec3 tree_center(origin.x + s_ChunkWidthAndHeight / 2, final_height + 4, origin.y + s_ChunkWidthAndHeight / 2);
			if (perlin_data.biome == Defs::Biome::Plains && i == tree_center.x && k == tree_center.z) {
				for (u32 p = 0; p < 4; p++)
					m_LocalBlocks.emplace_back(glm::vec3(i, final_height + p, k), Defs::BlockType::Wood);

				for (auto& vec : leaves_positions)
					m_LocalBlocks.emplace_back(tree_center + vec, Defs::BlockType::Leaves);
			}
		}
	}
	//Set chunk sector
	m_SectorIndex = Defs::ChunkSectorIndex(m_ChunkOrigin);
	Defs::g_PushedSections.insert(m_SectorIndex);

	/*static bool b = false;
	if (!b) {
		m_LocalDrops.emplace_back(glm::vec3(-80.0f, 100.0f, -80.0f), Defs::BlockType::Dirt);
		m_LocalDrops.clear();
		b = true;
	}*/
}

Chunk::Chunk(World& father, const Utils::Serializer& sz, u32 index) :
	m_RelativeWorld(father), m_State(GlCore::State::GlobalInstance()),
	m_SectorIndex(index), m_SelectedBlock(static_cast<u32>(-1))
{
	//Init water layer position vector
	m_WaterLayerPositions = std::make_shared<std::vector<glm::vec3>>();

	//Simply forward everithing to the deserializing operator
	Deserialize(sz);
}

Chunk::Chunk(Chunk&& rhs) noexcept :
	m_State(GlCore::State::GlobalInstance()), m_RelativeWorld(rhs.m_RelativeWorld)
{
	*this = std::move(rhs);
}

Chunk::~Chunk()
{
}

Chunk& Chunk::operator=(Chunk&& rhs) noexcept
{
	m_ChunkIndex = rhs.m_ChunkIndex;

	m_LocalBlocks = std::move(rhs.m_LocalBlocks);
	m_ChunkOrigin = rhs.m_ChunkOrigin;
	m_ChunkCenter = rhs.m_ChunkCenter;
	m_SectorIndex = rhs.m_SectorIndex;

	m_PlusX = rhs.m_PlusX;
	m_MinusX = rhs.m_MinusX;
	m_PlusZ = rhs.m_PlusZ;
	m_MinusZ = rhs.m_MinusZ;
	return *this;
}

void Chunk::InitGlobalNorms()
{
	//Determining if side chunk exist
	m_PlusX = m_RelativeWorld.IsChunk(*this, Defs::ChunkLocation::PlusX);
	m_MinusX = m_RelativeWorld.IsChunk(*this, Defs::ChunkLocation::MinusX);
	m_PlusZ = m_RelativeWorld.IsChunk(*this, Defs::ChunkLocation::PlusZ);
	m_MinusZ = m_RelativeWorld.IsChunk(*this, Defs::ChunkLocation::MinusZ);

	//Getting the relative ptr(can be nullptr obv)
	Chunk* chunk_plus_x = m_PlusX.has_value() ? &m_RelativeWorld.GetChunk(m_PlusX.value()) : nullptr;
	Chunk* chunk_minus_x = m_MinusX.has_value() ? &m_RelativeWorld.GetChunk(m_MinusX.value()) : nullptr;
	Chunk* chunk_plus_z = m_PlusZ.has_value() ? &m_RelativeWorld.GetChunk(m_PlusZ.value()) : nullptr;
	Chunk* chunk_minus_z = m_MinusZ.has_value() ? &m_RelativeWorld.GetChunk(m_MinusZ.value()) : nullptr;

	//Get normals
	const glm::vec3& pos_x = GlCore::g_PosX;
	const glm::vec3& neg_x = GlCore::g_NegX;
	const glm::vec3& pos_z = GlCore::g_PosZ;
	const glm::vec3& neg_z = GlCore::g_NegZ;

	//The following algorithm determines with precise accuracy and pretty fast speed which faces of each
	//block of the this chunk can be seen
	//The algorithm analyses each column of blocks in the 16x16 which can be found in the chunk and
	//from the top block of that column an index descends assigning to each side a normal
	//until a side block is found
	f32 local_x =  m_LocalBlocks[0].Position().x, local_z = m_LocalBlocks[0].Position().z;
	f32 last_y = 0.0f;
	s32 starting_index = 0;
	//+1 because we need to parse the last column
	for (s32 i = 0; i < m_LocalBlocks.size(); i++)
	{
		auto& loc_pos = m_LocalBlocks[i].Position();

		//Always include the last block
		if (i != m_LocalBlocks.size() - 1 &&
			loc_pos.x == local_x &&
			loc_pos.y - last_y < 1.5f &&
			loc_pos.z == local_z)
		{
			last_y = m_LocalBlocks[i].Position().y;
			continue;
		}

		local_x = m_LocalBlocks[i].Position().x;
		last_y = m_LocalBlocks[i].Position().y;
		local_z = m_LocalBlocks[i].Position().z;		

		u32 top_column_index = (i == m_LocalBlocks.size() - 1) ? i : i - 1;
		//The block at the top of the pile and the one at the bottom
		auto& top_block = m_LocalBlocks[top_column_index];
		auto& bot_block = m_LocalBlocks[starting_index];

		const glm::vec3& block_pos = top_block.Position();

		//if the blocks confines with another chunk right left front or back
		bool conf_rlfb[4];
		conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
		conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
		conf_rlfb[2] = block_pos.z == m_ChunkOrigin.y;
		conf_rlfb[3] = block_pos.z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1;

		//Upper block of a column is always visible from the top
		top_block.AddNormal(GlCore::g_PosY);

		s32 p = top_column_index;
		if (conf_rlfb[0] || conf_rlfb[1] || conf_rlfb[2] || conf_rlfb[3])
		{
			u32 blk_index;
			bool local_found = false, adjacent_found = false;
			//Checking also for neighbor chunks
			//Right
			while (p >= starting_index)
			{
				//If the block confines with a chunk that does not exist yet, we wont
				//push any normals, they will be deleted anyway when a new chunk spawns
				if (conf_rlfb[0] && !chunk_plus_x)
					break;
				
				//If we find the ground in a side check, break from the loop
				if (BorderCheck(chunk_plus_x, GlCore::g_PosX, p, starting_index, true))
					break;

				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index)
			{
				if (conf_rlfb[1] && !chunk_minus_x)
					break;

				if (BorderCheck(chunk_minus_x, GlCore::g_NegX, p, starting_index, false))
					break;

				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index)
			{
				if (conf_rlfb[3] && !chunk_plus_z)
					break;

				if (BorderCheck(chunk_plus_z, GlCore::g_PosZ, p, starting_index, true))
					break;

				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index)
			{
				if (conf_rlfb[2] && !chunk_minus_z)
					break;

				if (BorderCheck(chunk_minus_z, GlCore::g_NegZ, p, starting_index, false))
					break;

				p--;
			}
		}
		else
		{
			//A more slim implementation for the majority of the iterations
			//Right
			u32 blk_index;
			while (p >= starting_index)
			{
				if (!IsBlock(m_LocalBlocks[p].Position() + pos_x, starting_index, true, &blk_index)) {
					m_LocalBlocks[p].AddNormal(pos_x);
				}
				else {
					//If the current block is a leaf block, we wont discard lower placed blocks automatically
					//because we could find trunk or some other stuff
					if (m_LocalBlocks[blk_index].Type() != Defs::BlockType::Leaves)
						break;
				}

				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index)
			{
				if (!IsBlock(m_LocalBlocks[p].Position() + neg_x, starting_index, false, &blk_index)) {
					m_LocalBlocks[p].AddNormal(neg_x);
				}
				else {
					if (m_LocalBlocks[blk_index].Type() != Defs::BlockType::Leaves)
						break;
				}
				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index)
			{
				if (!IsBlock(m_LocalBlocks[p].Position() + pos_z, starting_index, true, &blk_index)) {
					m_LocalBlocks[p].AddNormal(pos_z);
				}
				else {
					if (m_LocalBlocks[blk_index].Type() != Defs::BlockType::Leaves)
						break;
				}
				
				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index)
			{
				if (!IsBlock(m_LocalBlocks[p].Position() + neg_z, starting_index, false, &blk_index)) {
					m_LocalBlocks[p].AddNormal(neg_z);
				}
				else {
					if (m_LocalBlocks[blk_index].Type() != Defs::BlockType::Leaves)
						break;
				}

				p--;
			}
		}
		//Push a normal at the bottom
		if(bot_block.Position().y != 0.0f)
			bot_block.AddNormal(GlCore::g_NegY);


		//Adding additional normals to side chunks, which were not considered because there hasn't
		//been a spawned chunk yet
		if (conf_rlfb[0] && chunk_plus_x)
		{
			u32 block_index;
			//Emplacing back normals from the first block higher than the current one 
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_plus_x->IsBlock(top_block.Position() + pos_x + begin_check, 0, true, &block_index))
			{
				chunk_plus_x->GetBlock(block_index).AddNormal(-1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[1] && chunk_minus_x)
		{
			u32 block_index;
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_minus_x->IsBlock(top_block.Position() + neg_x + begin_check, 0, true, &block_index))
			{
				chunk_minus_x->GetBlock(block_index).AddNormal(1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[2] && chunk_minus_z)
		{
			u32 block_index;
			glm::vec3 local_pos = GlCore::g_PosY;
			while (chunk_minus_z->IsBlock(top_block.Position() + neg_z + local_pos, 0, true, &block_index))
			{
				chunk_minus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, 1.0f);
				local_pos.y += 1.0f;
			}
		}

		if (conf_rlfb[3] && chunk_plus_z)
		{
			u32 block_index;
			glm::vec3 local_pos = GlCore::g_PosY;
			while (chunk_plus_z->IsBlock(top_block.Position() + pos_z + local_pos, 0, true, &block_index))
			{
				chunk_plus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, -1.0f);
				local_pos.y += 1.0f;
			}
		}

		starting_index = i;
	}
}

void Chunk::AddFreshNormals(Block& b)
{
	const auto& block_pos = b.Position();

	bool conf_rlfb[4];
	conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
	conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
	conf_rlfb[2] = block_pos.z == m_ChunkOrigin.y;
	conf_rlfb[3] = block_pos.z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1;

	auto local_lambda = [&](bool side_check, Defs::ChunkLocation cl, const glm::vec3& dir)
	{
		bool add_norm = true;
		if (side_check)
		{
			if (auto opt = m_RelativeWorld.IsChunk(*this, cl); opt.has_value())
			{
				if (m_RelativeWorld.GetChunk(opt.value()).IsBlock(block_pos + dir))
					add_norm = false;
			}
		}
		else
		{
			if (IsBlock(block_pos + dir))
				add_norm = false;
		}

		if (add_norm)
			b.AddNormal(dir);
	};

	local_lambda(conf_rlfb[0], Defs::ChunkLocation::PlusX, GlCore::g_PosX);
	local_lambda(conf_rlfb[1], Defs::ChunkLocation::MinusX, GlCore::g_NegX);
	local_lambda(false, Defs::ChunkLocation::None, GlCore::g_PosY);
	local_lambda(false, Defs::ChunkLocation::None, GlCore::g_NegY);
	local_lambda(conf_rlfb[2], Defs::ChunkLocation::PlusZ, GlCore::g_PosZ);
	local_lambda(conf_rlfb[3], Defs::ChunkLocation::MinusZ, GlCore::g_NegZ);
}

f32 Chunk::RayCollisionLogic(Inventory& inventory, bool left_click, bool right_click)
{
	f32 closest_selected_block_dist = INFINITY;
	//Reset the selection each time
	std::size_t vec_size = m_LocalBlocks.size();
	m_SelectedBlock = static_cast<u32>(-1);

	auto& camera_position = m_State.camera->GetPosition();
	auto& camera_direction = m_State.camera->GetFront();

	Defs::HitDirection selection = Defs::HitDirection::None;

	//Checking selection
	for (std::size_t i = 0; i < vec_size; ++i)
	{
		//Player view ray block collision
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be selected or seen
		if (!block.IsDrawable())
			continue;

		f32 dist = 0.0f;
		Defs::HitDirection local_selection = Defs::ViewBlockCollision(camera_position, camera_direction, block.Position(), dist);
		
		if (local_selection != Defs::HitDirection::None && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			selection = local_selection;
			m_SelectedBlock = i;
		}
	}

	if (Defs::g_ViewMode != Defs::ViewMode::Inventory) {
		//Logic which removes a block
		if (left_click && m_SelectedBlock != static_cast<u32>(-1))
		{
			const glm::vec3 position = m_LocalBlocks[m_SelectedBlock].Position();
			const Defs::BlockType type = m_LocalBlocks[m_SelectedBlock].Type();

			AddNewExposedNormals(m_LocalBlocks[m_SelectedBlock].Position());
			m_LocalBlocks.erase(m_LocalBlocks.begin() + m_SelectedBlock);
			m_SelectedBlock = static_cast<u32>(-1);

			//Push drop
			m_LocalDrops.emplace_back(position, type);
			//Signal block has been destroyed
			Defs::g_EnvironmentChange = true;
		}

		//Push a new block
		if (right_click && m_SelectedBlock != static_cast<u32>(-1))
		{
			//if the selected block isn't -1 that means selection is not NONE
			Defs::BlockType bt = Defs::g_InventorySelectedBlock;
			auto& block = m_LocalBlocks[m_SelectedBlock];

			//Remove one unit from the selection
			std::optional<InventoryEntry>& entry = inventory.HoveredFromSelector();
			if (entry.has_value()) {
				entry.value().block_count--;
				inventory.ClearUsedSlots();
			}

			switch (selection)
			{
			case Defs::HitDirection::PosX:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosX, bt);
				break;
			case Defs::HitDirection::NegX:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegX, bt);
				break;
			case Defs::HitDirection::PosY:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosY, bt);
				break;
			case Defs::HitDirection::NegY:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegY, bt);
				break;
			case Defs::HitDirection::PosZ:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosZ, bt);
				break;
			case Defs::HitDirection::NegZ:
				m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegZ, bt);
				break;

			case Defs::HitDirection::None:
				//UNREACHABLE
				break;
			}

			AddFreshNormals(m_LocalBlocks.back());
			Defs::g_EnvironmentChange = true;
		}
	}

	return closest_selected_block_dist;
}

void Chunk::BlockCollisionLogic(glm::vec3& position)
{
	//Player collision check with the environment
	if (glm::length(glm::vec2(position.x, position.z) - glm::vec2(m_ChunkCenter.x, m_ChunkCenter.z)) > 12.0f)
		return;

	f32 initial_treshold = 0.01f;
	//TODO play with the values to fix clipping problems
	for (u32 i = 0; i < 3; i++) {
		f32& cd = Defs::g_PlayerAxisMapping[i];
		//accelerated increase in this axis speed
		if (cd > 1.0f) {
			cd = 1.0f;
		}
		else if (cd > initial_treshold && cd < 1.0f) {
			cd += 0.475f;
		}
		else {
			//Four frames to get there
			cd += initial_treshold / 4.0f;
		}
	}

	for (u32 i = 0; i < m_LocalBlocks.size(); i++) {
		if (!m_LocalBlocks[i].HasNormals())
			continue;

		const glm::vec3& block_pos = m_LocalBlocks[i].Position();
		glm::vec3 diff = position - block_pos;
		glm::vec3 abs_diff = glm::abs(diff);
		if (abs_diff.x < 1.0f && abs_diff.y < 1.0f && abs_diff.z < 1.0f) {
			//Simple collision solver
			if (abs_diff.z > abs_diff.x && abs_diff.z > abs_diff.y) {
				position.z = position.z < block_pos.z ? block_pos.z - 1.0f : block_pos.z + 1.0f;
				Defs::g_PlayerAxisMapping.z = 0.0f;
			}

			if (abs_diff.x > abs_diff.y && abs_diff.x > abs_diff.z) {
				position.x = position.x < block_pos.x ? block_pos.x - 1.0f : block_pos.x + 1.0f;
				Defs::g_PlayerAxisMapping.x = 0.0f;
			}

			if (abs_diff.y > abs_diff.z && abs_diff.y > abs_diff.x) {
				Defs::g_PlayerAxisMapping.y = 0.0f;
				if (position.y < block_pos.y) {
					position.y = block_pos.y - 1.0f;
				}
				else {
					position.y = block_pos.y + 1.0f;
					Defs::jump_data = { 0.0f, true };
				}
			}
		}
	}
}

void Chunk::UpdateBlocks(Inventory& inventory, f32 elapsed_time)
{
	auto& camera_position = m_State.camera->GetPosition();

	//Update each single block
	for (auto& block : m_LocalBlocks)
		block.UpdateRenderableSides(camera_position);

	//Update local drops
	for (auto iter = m_LocalDrops.begin(); iter != m_LocalDrops.end(); ++iter) {
		auto& drop = *iter;
		drop.Update(this, elapsed_time);
		drop.UpdateModel(elapsed_time);

		if (glm::length(drop.Position() - m_State.camera->GetPosition()) < 1.0f) {
			inventory.AddToNewSlot(drop.Type());
			bool last_flag = iter == std::prev(m_LocalDrops.end());
			iter = m_LocalDrops.erase(iter);

			//need this otherwise the program will crash for incrementing a dangling iterator
			if (last_flag)
				break;
		}
	}
}

bool Chunk::IsChunkRenderable() const
{
	auto& camera_position = m_State.camera->GetPosition();

	//This algorithm does not take account for the player altitude in space
	glm::vec2 cam_pos(camera_position.x, camera_position.z);
	glm::vec2 chunk_center_pos(m_ChunkCenter.x, m_ChunkCenter.z);
	return (glm::length(cam_pos - chunk_center_pos) < Defs::g_ChunkRenderingDistance);
}

bool Chunk::IsChunkVisible() const
{
	auto& camera_position = m_State.camera->GetPosition();
	auto& camera_direction = m_State.camera->GetFront();

	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - camera_position);
	return (glm::dot(camera_to_midway, camera_direction) > 0.4f ||
		glm::length(camera_position - m_ChunkCenter) < s_DiagonalLenght + Defs::g_CameraCompensation);
}

bool Chunk::IsChunkVisibleByShadow() const
{
	glm::vec3 camera_position = m_State.camera->GetPosition() + GlCore::g_FramebufferPlayerOffset;
	const glm::vec3& camera_direction = GlCore::g_NegY;
	
	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - camera_position);
	return (glm::dot(camera_to_midway, camera_direction) > 0.5f);
}

void Chunk::RemoveBorderNorm(const glm::vec3& norm)
{
	//Function that determines if a block normal towards the chunk can be removed,
	//depending on if there is a block next to him in the desired direction.
	//If there is no block, the normal isn't going to be removed
	auto erase_flanked_normal = [&](Block& block, const glm::vec3& vec, const Defs::ChunkLocation& loc)
	{
		u32 index = GetLoadedChunk(loc).value_or(0);
		glm::vec3 block_pos = block.Position() + vec;
		bool is_block = m_RelativeWorld.GetChunk(index).IsBlock(block_pos);

		if (is_block)
			block.RemoveNormal(norm);
	};

	//I remind you that the y component in the chunk origin is actually the world z component
	if (norm == glm::vec3(1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::PlusX);
	}
	else if (norm == glm::vec3(-1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().x == m_ChunkOrigin.x)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::MinusX);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, 1.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::PlusZ);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, -1.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().z == m_ChunkOrigin.y)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::MinusZ);
	}

}

const glm::vec2& Chunk::GetChunkOrigin() const
{
	return m_ChunkOrigin;
}

const std::optional<u32>& Chunk::GetLoadedChunk(const Defs::ChunkLocation& cl) const
{
	switch (cl)
	{
	case Defs::ChunkLocation::PlusX:
		return m_PlusX;
	case Defs::ChunkLocation::MinusX:
		return m_MinusX;
	case Defs::ChunkLocation::PlusZ:
		return m_PlusZ;
	case Defs::ChunkLocation::MinusZ:
		return m_MinusZ;
	}

	return std::nullopt;
}

void Chunk::SetLoadedChunk(const Defs::ChunkLocation& cl, u32 value)
{
	switch (cl)
	{
	case Defs::ChunkLocation::PlusX:
		m_PlusX = value;
		break;
	case Defs::ChunkLocation::MinusX:
		m_MinusX = value;
		break;
	case Defs::ChunkLocation::PlusZ:
		m_PlusZ = value;
		break;
	case Defs::ChunkLocation::MinusZ:
		m_MinusZ = value;
		break;
	}
}

void Chunk::ForwardRenderableData(glm::vec3*& position_buf, u32*& texindex_buf, u32& count, bool depth_buf_draw, bool selected) const
{
	//We let this algorithm fill the buffers of the instanced shader attributes
	for (std::size_t i = 0; i < m_LocalBlocks.size(); ++i)
	{
		auto& block = m_LocalBlocks[i];
		if (!depth_buf_draw)
		{
			//For standard drawcalls, we care about visible blocks
			if (!block.IsDrawable())
				continue;

			texindex_buf[count] = static_cast<u32>(block.Type());
			position_buf[count++] = block.Position();

			//Handle selection by pushing another matrix and a null index
			if (selected && i == s_InternalSelectedBlock)
			{
				texindex_buf[count] = 256 + texindex_buf[count - 1];
				position_buf[count] = position_buf[count - 1];
				count++;
			}

			if (count == GlCore::g_MaxRenderedObjCount)
				GlCore::DispatchBlockRendering(position_buf, texindex_buf, count);
		}
		else
		{
			//For depth drawcalls, we include every surface block
			if (!block.HasNormals())
				continue;

			position_buf[count++] = block.Position();

			if (count == GlCore::g_MaxRenderedObjCount)
				GlCore::DispatchDepthRendering(position_buf, count);
		}
	}

	//Send water layer if present
	if (!depth_buf_draw && !m_WaterLayerPositions->empty())
		m_RelativeWorld.PushWaterLayer(m_WaterLayerPositions);
}

void Chunk::RenderDrops()
{
	for (auto& drop : m_LocalDrops)
		drop.Render();
}

void Chunk::AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check)
{
	auto compute_new_normals = [&](const glm::vec3& pos, const glm::vec3& norm)
	{
		glm::vec3 neighbor_pos = pos + norm;
		auto local_iter = std::find_if(m_LocalBlocks.begin(), m_LocalBlocks.end(),
			[neighbor_pos](const Block& b) {return b.Position() == neighbor_pos; });

		//Behaviour between chunks
		if (local_iter == m_LocalBlocks.end())
		{
			if (!side_chunk_check)
			{
				if (m_PlusX.has_value() && norm == GlCore::g_PosX)
					m_RelativeWorld.GetChunk(m_PlusX.value()).AddNewExposedNormals(pos, true);
				if (m_MinusX.has_value() && norm == GlCore::g_NegX)
					m_RelativeWorld.GetChunk(m_MinusX.value()).AddNewExposedNormals(pos, true);
				if (m_PlusZ.has_value() && norm == GlCore::g_PosZ)
					m_RelativeWorld.GetChunk(m_PlusZ.value()).AddNewExposedNormals(pos, true);
				if (m_MinusZ.has_value() && norm == GlCore::g_NegZ)
					m_RelativeWorld.GetChunk(m_MinusZ.value()).AddNewExposedNormals(pos, true);
			}
		}
		else
		{
			local_iter->AddNormal(-norm);
		}
	};

	compute_new_normals(block_pos, GlCore::g_PosX);
	compute_new_normals(block_pos, GlCore::g_NegX);
	compute_new_normals(block_pos, GlCore::g_PosY);
	compute_new_normals(block_pos, GlCore::g_NegY);
	compute_new_normals(block_pos, GlCore::g_PosZ);
	compute_new_normals(block_pos, GlCore::g_NegZ);
}

u32 Chunk::LastSelectedBlock() const
{
	return m_SelectedBlock;
}

u32 Chunk::SectorIndex() const
{
	return m_SectorIndex;
}

u32 Chunk::Index() const
{
	return m_ChunkIndex;
}

glm::vec3 Chunk::GetHalfWayVector()
{
	return glm::vec3(s_ChunkWidthAndHeight / 2.0f, s_ChunkDepth / 2.0f, s_ChunkWidthAndHeight / 2.0f);
}

const Utils::Serializer& Chunk::Serialize(const Utils::Serializer& sz)
{
	//Serializing components
	//At first we tell how many blocks and water layers the chunk has
	sz& m_LocalBlocks.size();
	sz& m_WaterLayerPositions->size();

	sz& m_ChunkIndex;
	//World address does not need to be serialized

	if (!m_LocalBlocks.empty())
	{
		auto& base_vec = m_LocalBlocks[0].Position();
		//We serialize only a single 12bytes position vector,
		//so the other can be stored as 3bytes vector offsets
		sz& base_vec.x& base_vec.y& base_vec.z;

		for (auto& block : m_LocalBlocks)
			block.Serialize(sz, base_vec);
	}

	//Serialize water layers
	for (auto& layer : *m_WaterLayerPositions)
		sz& layer.x& layer.y& layer.z;

	sz& m_ChunkOrigin.x & m_ChunkOrigin.y;
	//m_ChunkCenter can be calculated from m_ChunkOrigin
	//m_SelectedBlock does not need to be serialized

	sz& m_PlusX.value_or(-1)
		& m_MinusX.value_or(-1)
		& m_PlusZ.value_or(-1)
		& m_MinusZ.value_or(-1);

	return sz;
}

const Utils::Serializer& Chunk::Deserialize(const Utils::Serializer& sz)
{
	std::size_t blk_vec_size;
	std::size_t water_layer_size;
	glm::vec3 base_vec;
	u32 adj_chunks[4];

	sz% blk_vec_size;
	sz% water_layer_size;

	sz% m_ChunkIndex;

	if (blk_vec_size != 0)
	{
		m_LocalBlocks.clear();

		sz% base_vec.x% base_vec.y% base_vec.z;

		glm::u8vec3 v;
		u8 norm_payload, block_type;

		for (u32 i = 0; i < blk_vec_size; i++)
		{
			sz% v.x% v.y% v.z;
			sz% norm_payload;
			sz% block_type;

			glm::vec3 original_vec = base_vec + static_cast<glm::vec3>(v);
			m_LocalBlocks.emplace_back(original_vec, static_cast<Defs::BlockType>(block_type));
			auto& bb = m_LocalBlocks.back();
			
			Utils::Bitfield<6> bf( {norm_payload} );
			for (u32 i = 0; i < bf.Size(); i++)
				if (bf[i])
					bb.AddNormal(Block::NormalForIndex(i));
		}
	}

	//Deserialize water layers
	for (u32 i = 0; i < water_layer_size; i++)
	{
		f32 x, y, z;
		sz% x; sz% y; sz% z;
		m_WaterLayerPositions->emplace_back(x, y, z);
	}

	sz% m_ChunkOrigin.x% m_ChunkOrigin.y;

	//Calculate chunk center
	glm::vec3 chunk_origin_3d(m_ChunkOrigin.x, 0.0f, m_ChunkOrigin.y);
	m_ChunkCenter = chunk_origin_3d + GetHalfWayVector();

	sz% adj_chunks[0] % adj_chunks[1] % adj_chunks[2] % adj_chunks[3];

	m_PlusX = adj_chunks[0] != -1 ? std::make_optional(adj_chunks[0]) : std::nullopt;
	m_MinusX = adj_chunks[1] != -1 ? std::make_optional(adj_chunks[1]) : std::nullopt;
	m_PlusZ = adj_chunks[2] != -1 ? std::make_optional(adj_chunks[2]) : std::nullopt;
	m_MinusZ = adj_chunks[3] != -1 ? std::make_optional(adj_chunks[3]) : std::nullopt;

	return sz;
}

bool Chunk::IsBlock(const glm::vec3& pos, s32 starting_index, bool search_towards_end, u32* block_index) const
{
	if (starting_index < 0 || starting_index >= m_LocalBlocks.size())
		throw std::runtime_error("Starting index out of bounds");

	if (search_towards_end)
	{
		//Searching before in the defined batch
		for (s32 i = starting_index; i < m_LocalBlocks.size(); i++)
		{
			if (m_LocalBlocks[i].Position() == pos)
			{
				if (block_index)
					*block_index = i;

				return true;
			}
		}

	}
	else
	{
		for (s32 i = starting_index; i >= 0; i--)
		{
			if (m_LocalBlocks[i].Position() == pos)
			{
				if (block_index)
					*block_index = i;

				return true;
			}
		}
	}

	return false;
}

Block& Chunk::GetBlock(u32 index)
{
	return m_LocalBlocks[index];
}

const Block& Chunk::GetBlock(u32 index) const
{
	return m_LocalBlocks[index];
}

bool Chunk::BorderCheck(Chunk* chunk, const glm::vec3& pos, u32 top_index, u32 bot_index, bool search_dir)
{
	bool local_found = false, adjacent_found = false;
	u32 blk_index;
	local_found = IsBlock(m_LocalBlocks[top_index].Position() + pos, bot_index, search_dir, &blk_index);
	if (!local_found)
			adjacent_found = chunk && chunk->IsBlock(m_LocalBlocks[top_index].Position() + pos, 0, true, &blk_index);

	if (local_found) {
		if (m_LocalBlocks[blk_index].Type() != Defs::BlockType::Leaves)
			return true;
	}
	else if (adjacent_found) {
		if (chunk->GetBlock(blk_index).Type() != Defs::BlockType::Leaves)
			return true;
	}
	else {
		m_LocalBlocks[top_index].AddNormal(pos);
	}

	return false;
}


