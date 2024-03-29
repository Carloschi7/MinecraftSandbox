#include "Chunk.h"
#include "World.h"
#include "InventorySystem.h"
#include "Renderer.h"
#include <algorithm>
#include <chrono>

//Half a chunk's diagonal
f32 Chunk::s_DiagonalLenght = 0.0f;
u32 Chunk::s_InternalSelectedBlock = static_cast<u32>(-1);

ChunkGeneration ComputeGeneration(Defs::WorldSeed& seed, f32 origin_x, f32 origin_z) 
{
	ChunkGeneration gen;
	gen.lower = 255;
	for (u8 i = 0; i < Chunk::s_ChunkWidthAndHeight; i++)
	{
		for (u8 k = 0; k < Chunk::s_ChunkWidthAndHeight; k++)
		{
			f32 fx = static_cast<f32>(origin_x + (float)i), fz = static_cast<f32>(origin_z + (float)k);
			auto perlin_data = Defs::PerlNoise::GetBlockAltitude(fx, fz, seed);
			u8 final_height = (Chunk::s_ChunkDepth - 10) + std::roundf(perlin_data.altitude * 8.0f);
			gen.heights[i * Chunk::s_ChunkWidthAndHeight + k] = final_height;
			gen.biomes[i * Chunk::s_ChunkWidthAndHeight + k] = perlin_data.biome;
			gen.in_water[i * Chunk::s_ChunkWidthAndHeight + k] = perlin_data.in_water;

			if (final_height < gen.lower)
				gen.lower = final_height;
		}
	}
	return gen;
}

Chunk::Chunk(World& father, glm::vec2 origin)
	:m_RelativeWorld(father), m_State(*GlCore::pstate),
	m_ChunkOrigin({origin.x, 0.0f, origin.y}), m_SelectedBlock(static_cast<u32>(-1)),
	m_ChunkCenter(0.0f), m_SectorIndex(0)
{
	//Assigning chunk index
	m_ChunkIndex = Defs::g_ChunkProgIndex++;
	m_ChunkCenter = m_ChunkOrigin + GetHalfWayVector();

	if (s_DiagonalLenght == 0.0f)
	{
		f32 lower_diag_squared = 2 * glm::pow(s_ChunkWidthAndHeight, 2);
		s_DiagonalLenght = glm::sqrt(lower_diag_squared + glm::pow(s_ChunkDepth, 2)) * 0.5f;
	}

	//Chunk tree leaves if present
	Utils::Vector<glm::vec3> leaves_positions = Defs::GenerateRandomFoliage(
		m_RelativeWorld.relative_leaves_positions,
		m_RelativeWorld.random_engine);

	//Create the terrain generation for this chunk if it was not already computed and
	//figure out which is the lowest level of neighbor chunks to figure out how 
	//many blocks need to be added


	auto& generations = m_RelativeWorld.perlin_generations;
	ChunkGeneration gen;
	if (generations.find(Hash(origin)) == generations.end()) {
		gen = ComputeGeneration(m_RelativeWorld.Seed(), m_ChunkOrigin.x, m_ChunkOrigin.z);
		generations[Hash(origin)] = gen;
	}
	else {
		gen = generations[Hash(origin)];
	}
	u8 lower_height = gen.lower - 2;

	glm::vec2 vecs[] = { 
		glm::vec2(origin.x + 16.0f, origin.y), 
		glm::vec2(origin.x - 16.0f, origin.y),
		glm::vec2(origin.x, origin.y + 16.0f),
		glm::vec2(origin.x, origin.y - 16.0f) };

	for (u32 i = 0; i < 4; i++) {
		u64 current_hash = Hash(vecs[i]);
		if (generations.find(current_hash) != generations.end()) {
			if (generations[current_hash].lower < lower_height)
				lower_height = generations[current_hash].lower;
		}
		else {
			generations[current_hash] = ComputeGeneration(m_RelativeWorld.Seed(), vecs[i].x, vecs[i].y);
			if (generations[current_hash].lower < lower_height)
				lower_height = generations[current_hash].lower;
		}
	}

	lower_threshold = lower_height;

	//Insert the y coordinates consecutively to allow the
	//normal insertion algorithm later
	for (u8 i = 0; i < s_ChunkWidthAndHeight; i++)
	{
		for (u8 k = 0; k < s_ChunkWidthAndHeight; k++)
		{
			f32 fx = static_cast<f32>(m_ChunkOrigin.x + (float)i), fy = static_cast<f32>(m_ChunkOrigin.z + (float)k);
			u8 final_height = gen.heights[i * s_ChunkWidthAndHeight + k];
			Defs::Biome biome = gen.biomes[i * s_ChunkWidthAndHeight + k];
			bool in_water = gen.in_water[i * s_ChunkWidthAndHeight + k];

			for (u8 j = lower_height; j < final_height; j++)
			{
				switch (biome)
				{
				case Defs::Biome::Plains:
					chunk_blocks.emplace_back(glm::u8vec3(i, j, k), (j == final_height - 1) ?
						Defs::Item::Grass : Defs::Item::Dirt);
					break;
				case Defs::Biome::Desert:
					chunk_blocks.emplace_back(glm::u8vec3(i, j, k), Defs::Item::Sand);
					break;
				}
			}

			if (in_water)
			{
				f32 water_level = Defs::WaterRegionLevel(fx, fy, m_RelativeWorld.Seed(), m_RelativeWorld.pushed_areas);
				u32 water_height = (s_ChunkDepth - 10) + std::roundf(water_level * 8.0f) - 1;

				if (water_height >= final_height)
					m_WaterLayerPositions.push_back(glm::vec3(fx, water_height, fy));

				continue;
			}

			glm::vec3 tree_center(s_ChunkWidthAndHeight / 2, final_height + 4, s_ChunkWidthAndHeight / 2);
			if (biome == Defs::Biome::Plains && i == tree_center.x && k == tree_center.z) {
				for (u32 p = 0; p < 4; p++)
					chunk_blocks.emplace_back(glm::vec3(i, final_height + p, k), Defs::Item::Wood);

				for (auto& vec : leaves_positions)
					chunk_blocks.emplace_back(static_cast<glm::u8vec3>(tree_center + vec), Defs::Item::Leaves);
			}
		}
	}
	//Set chunk sector
	m_SectorIndex = Defs::ChunkSectorIndex({m_ChunkOrigin.x, m_ChunkOrigin.z});
	Defs::g_PushedSections.insert(m_SectorIndex);
}

Chunk::Chunk(World& father, const Utils::Serializer& sz, u32 index) :
	m_RelativeWorld(father), m_State(*GlCore::pstate),
	m_SectorIndex(index), m_SelectedBlock(static_cast<u32>(-1))
{
	//Simply forward everithing to the deserializing operator
	Deserialize(sz);
}

Chunk::Chunk(Chunk&& rhs) noexcept :
	m_State(*GlCore::pstate), m_RelativeWorld(rhs.m_RelativeWorld)
{
	*this = std::move(rhs);
}

Chunk::~Chunk()
{
}

Chunk& Chunk::operator=(Chunk&& rhs) noexcept
{
	m_ChunkIndex = rhs.m_ChunkIndex;

	chunk_blocks = std::move(rhs.chunk_blocks);
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
	Pointer<Chunk> chunk_plus_x = m_PlusX.has_value() ? m_RelativeWorld.GetChunk(m_PlusX.value()) : nullptr;
	Pointer<Chunk> chunk_minus_x = m_MinusX.has_value() ? m_RelativeWorld.GetChunk(m_MinusX.value()) : nullptr;
	Pointer<Chunk> chunk_plus_z = m_PlusZ.has_value() ? m_RelativeWorld.GetChunk(m_PlusZ.value()) : nullptr;
	Pointer<Chunk> chunk_minus_z = m_MinusZ.has_value() ? m_RelativeWorld.GetChunk(m_MinusZ.value()) : nullptr;

	//Get normals
	const glm::vec3 pos_x(1.0f, 0.0f, 0.0f);
	const glm::vec3 neg_x(-1.0f, 0.0f, 0.0f);
	const glm::vec3 pos_y(0.0f, 1.0f, 0.0f);
	const glm::vec3 neg_y(0.0f, -1.0f, 0.0f);
	const glm::vec3 pos_z(0.0f, 0.0f, 1.0f);
	const glm::vec3 neg_z(0.0f, 0.0f, -1.0f);

	//The following algorithm determines with precise accuracy and pretty fast speed which faces of each
	//block of the this chunk can be seen
	//The algorithm analyses each column of blocks in the 16x16 which can be found in the chunk and
	//from the top block of that column an index descends assigning to each side a normal
	//until a side block is found
	f32 local_x = m_ChunkOrigin.x + chunk_blocks[0].position.x, local_z = m_ChunkOrigin.z + chunk_blocks[0].position.z;
	s32 starting_index = 0;
	//+1 because we need to parse the last column
	for (s32 i = 0; i < chunk_blocks.size(); i++)
	{
		glm::vec3 loc_pos = ToWorld(chunk_blocks[i].position);

		//Always include the last block
		if (i != chunk_blocks.size() - 1 &&
			loc_pos.x == local_x &&
			loc_pos.z == local_z)
		{
			continue;
		}

		local_x = loc_pos.x;
		local_z = loc_pos.z;		

		u32 top_column_index = (i == chunk_blocks.size() - 1) ? i : i - 1;
		//The block at the top of the pile and the one at the bottom
		auto& top_block = chunk_blocks[top_column_index];
		auto& bot_block = chunk_blocks[starting_index];

		const glm::vec3 block_pos = ToWorld(top_block.position);

		//if the blocks confines with another chunk right left front or back
		bool conf_rlfb[4];
		conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
		conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
		conf_rlfb[2] = block_pos.z == m_ChunkOrigin.z;
		conf_rlfb[3] = block_pos.z == m_ChunkOrigin.z + s_ChunkWidthAndHeight - 1;

		//Upper block of a column is always visible from the top
		top_block.AddNormal(pos_y);

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
				if (conf_rlfb[0] && !chunk_plus_x.Valid())
					break;
				
				//If we find the ground in a side check, break from the loop
				if (BorderCheck(chunk_plus_x.Raw(), pos_x, p, starting_index, true))
					break;

				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index)
			{
				if (conf_rlfb[1] && !chunk_minus_x.Valid())
					break;

				if (BorderCheck(chunk_minus_x.Raw(), neg_x, p, starting_index, false))
					break;

				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index)
			{
				if (conf_rlfb[3] && !chunk_plus_z.Valid())
					break;

				if (BorderCheck(chunk_plus_z.Raw(), pos_z, p, starting_index, true))
					break;

				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index)
			{
				if (conf_rlfb[2] && !chunk_minus_z.Valid())
					break;

				if (BorderCheck(chunk_minus_z.Raw(), neg_z, p, starting_index, false))
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
				auto pos = ToWorld(chunk_blocks[p].position) + pos_x;
				if (!IsBlock(pos, starting_index, true, &blk_index)) {
					chunk_blocks[p].AddNormal(pos_x);
				}
				else {
					//If the current block is a leaf block, we wont discard lower placed blocks automatically
					//because we could find trunk or some other stuff
					if (chunk_blocks[blk_index].Type() != Defs::Item::Leaves)
						break;
				}

				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index)
			{
				auto pos = ToWorld(chunk_blocks[p].position) + neg_x;
				if (!IsBlock(pos, starting_index, false, &blk_index)) {
					chunk_blocks[p].AddNormal(neg_x);
				}
				else {
					if (chunk_blocks[blk_index].Type() != Defs::Item::Leaves)
						break;
				}
				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index)
			{
				auto pos = ToWorld(chunk_blocks[p].position) + pos_z;
				if (!IsBlock(pos, starting_index, true, &blk_index)) {
					chunk_blocks[p].AddNormal(pos_z);
				}
				else {
					if (chunk_blocks[blk_index].Type() != Defs::Item::Leaves)
						break;
				}
				
				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index)
			{
				auto pos = ToWorld(chunk_blocks[p].position) + neg_z;
				if (!IsBlock(pos, starting_index, false, &blk_index)) {
					chunk_blocks[p].AddNormal(neg_z);
				}
				else {
					if (chunk_blocks[blk_index].Type() != Defs::Item::Leaves)
						break;
				}

				p--;
			}
		}
		//Push a normal at the bottom
		if(bot_block.position.y != lower_threshold)
			bot_block.AddNormal(neg_y);


		//Adding additional normals to side chunks, which were not considered because there hasn't
		//been a spawned chunk yet
		if (conf_rlfb[0] && chunk_plus_x.Valid())
		{
			u32 block_index;
			//Emplacing back normals from the first block higher than the current one 
			glm::vec3 begin_check = pos_y;
			while (chunk_plus_x->IsBlock(ToWorld(top_block.position) + pos_x + begin_check, 0, true, &block_index))
			{
				chunk_plus_x->GetBlock(block_index).AddNormal(-1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[1] && chunk_minus_x.Valid())
		{
			u32 block_index;
			glm::vec3 begin_check = pos_y;
			while (chunk_minus_x->IsBlock(ToWorld(top_block.position) + neg_x + begin_check, 0, true, &block_index))
			{
				chunk_minus_x->GetBlock(block_index).AddNormal(1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[2] && chunk_minus_z.Valid())
		{
			u32 block_index;
			glm::vec3 local_pos = pos_y;
			while (chunk_minus_z->IsBlock(ToWorld(top_block.position) + neg_z + local_pos, 0, true, &block_index))
			{
				chunk_minus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, 1.0f);
				local_pos.y += 1.0f;
			}
		}

		if (conf_rlfb[3] && chunk_plus_z.Valid())
		{
			u32 block_index;
			glm::vec3 local_pos = pos_y;
			while (chunk_plus_z->IsBlock(ToWorld(top_block.position) + pos_z + local_pos, 0, true, &block_index))
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
	const glm::vec3 block_pos = ToWorld(b.position);

	bool conf_rlfb[4];
	conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
	conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
	conf_rlfb[2] = block_pos.z == m_ChunkOrigin.z;
	conf_rlfb[3] = block_pos.z == m_ChunkOrigin.z + s_ChunkWidthAndHeight - 1;

	auto local_lambda = [&](bool side_check, Defs::ChunkLocation cl, const glm::vec3& dir)
	{
		bool add_norm = true;
		if (side_check)
		{
			if (auto opt = m_RelativeWorld.IsChunk(*this, cl); opt.has_value())
			{
				Pointer<Chunk> chunk = m_RelativeWorld.GetChunk(opt.value());
				if (chunk->IsBlock(block_pos + dir))
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

	local_lambda(conf_rlfb[0], Defs::ChunkLocation::PlusX, glm::vec3( 1.0f, 0.0f, 0.0f));
	local_lambda(conf_rlfb[1], Defs::ChunkLocation::MinusX, glm::vec3(-1.0f, 0.0f, 0.0f));
	local_lambda(false, Defs::ChunkLocation::None, glm::vec3( 0.0f, 1.0f, 0.0f));
	local_lambda(false, Defs::ChunkLocation::None, glm::vec3(0.0f, -1.0f, 0.0f));
	local_lambda(conf_rlfb[2], Defs::ChunkLocation::PlusZ, glm::vec3( 0.0f, 0.0f, 1.0f));
	local_lambda(conf_rlfb[3], Defs::ChunkLocation::MinusZ, glm::vec3(0.0f, 0.0f, -1.0f));
}

std::pair<f32, Defs::HitDirection> Chunk::RayCollisionLogic(const glm::vec3& camera_position, const glm::vec3& camera_direction)
{
	f32 closest_selected_block_dist = INFINITY;
	//Reset the selection each time
	u64 vec_size = chunk_blocks.size();
	m_SelectedBlock = static_cast<u32>(-1);

	Defs::HitDirection selection = Defs::HitDirection::None;

	//Checking selection
	for (u64 i = 0; i < vec_size; ++i)
	{
		//Player view ray block collision
		auto& block = chunk_blocks[i];
		//Discard automatically blocks which cant be selected or seen
		if (!block.IsDrawable())
			continue;

		f32 dist = 0.0f;
		//Target only forward blocks
		glm::vec3 position = ToWorld(block.position);
		f32 block_dot_value = glm::dot(camera_direction, glm::normalize(position - camera_position));
		if (block_dot_value >= 0.0f) {
			Defs::HitDirection local_selection = Defs::ViewBlockCollision(camera_position, camera_direction, position, dist);
			if (local_selection != Defs::HitDirection::None && dist < closest_selected_block_dist)
			{
				closest_selected_block_dist = dist;
				selection = local_selection;
				m_SelectedBlock = i;
			}
		}
	}

	return { closest_selected_block_dist, selection };
}

bool Chunk::BlockCollisionLogic(glm::vec3& position)
{
	f32 clip_threshold = 0.9f;
	bool result = false;
	for (u32 i = 0; i < chunk_blocks.size(); i++) {
		if (!chunk_blocks[i].HasNormals())
			continue;

		const glm::vec3 block_pos = ToWorld(chunk_blocks[i].position);
		glm::vec3 diff = position - block_pos;
		glm::vec3 abs_diff = glm::abs(diff);
		//Adjusting the y collision components because the player has an height of 2
		f32 y_halved = diff.y < 0.0f ? abs_diff.y : abs_diff.y / 2.0f;
		f32 y_hit_threshold = diff.y < 0.0f ? 1.0f : 2.0f;
		if (abs_diff.x < clip_threshold && abs_diff.y < clip_threshold * y_hit_threshold && abs_diff.z < clip_threshold) {
			//Simple collision solver
			if (Defs::g_PlayerSpeed != 0.0f) {
				if (abs_diff.z > abs_diff.x && abs_diff.z > y_halved) {
					position.z = position.z < block_pos.z ? block_pos.z - clip_threshold : block_pos.z + clip_threshold;
					Defs::g_PlayerAxisMapping.z = 0.0f;
					result = true;
				}

				if (abs_diff.x > y_halved && abs_diff.x > abs_diff.z) {
					position.x = position.x < block_pos.x ? block_pos.x - clip_threshold : block_pos.x + clip_threshold;
					Defs::g_PlayerAxisMapping.x = 0.0f;
					result = true;
				}
			}

			if (y_halved > abs_diff.z && y_halved > abs_diff.x) {
				Defs::g_PlayerAxisMapping.y = 0.0f;
				if (position.y < block_pos.y) {
					position.y = block_pos.y - clip_threshold;
					Defs::jump_data = { 0.0f, false };
				}
				else {
					position.y = block_pos.y + clip_threshold * 2.0f;
					Defs::jump_data = { 0.0f, true };
				}
			}
		}
	}

	return result;
}

void Chunk::UpdateBlocks(Inventory& inventory, f32 elapsed_time)
{
	auto& camera_position = m_State.camera->GetPosition();

	//Update each single block
	for (auto& block : chunk_blocks)
		block.UpdateRenderableSides(this, camera_position);

	//Update local drops
	for (auto iter = m_LocalDrops.begin(); iter != m_LocalDrops.end(); ++iter) {
		auto& drop = *iter;
		drop.Update(this, elapsed_time);
		drop.UpdateModel(elapsed_time);

		//Done because the actual player is the head position, we want to grab these when we
		//step on them
		glm::vec3 ground_position = m_State.camera->GetPosition();
		ground_position.y -= 1.0f;
		if (glm::length(drop.position - ground_position) < 1.0f) {
			inventory.AddToNewSlot(drop.Type());
			bool last_flag = iter == std::prev(m_LocalDrops.end());
			iter = m_LocalDrops.erase(iter);

			//need this otherwise the program will crash for incrementing a dangling iterator
			if (last_flag)
				break;
		}
	}
}

bool Chunk::IsChunkRenderable(const glm::vec3& camera_position) const
{
	//This algorithm does not take account for the player altitude in space
	glm::vec2 cam_pos(camera_position.x, camera_position.z);
	glm::vec2 chunk_center_pos(m_ChunkCenter.x, m_ChunkCenter.z);
	return (glm::length(cam_pos - chunk_center_pos) < Defs::g_ChunkRenderingDistance);
}

bool Chunk::IsChunkVisible(const glm::vec3& camera_position, const glm::vec3& camera_direction) const
{
	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - camera_position);
	return (glm::dot(camera_to_midway, camera_direction) > 0.4f ||
		glm::length(camera_position - m_ChunkCenter) < s_DiagonalLenght + Defs::g_CameraCompensation);
}

bool Chunk::IsChunkVisibleByShadow(const glm::vec3& camera_position, const glm::vec3& camera_direction) const
{
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
		glm::vec3 block_pos = ToWorld(block.position) + vec;
		bool is_block = m_RelativeWorld.GetChunk(index)->IsBlock(block_pos);

		if (is_block)
			block.RemoveNormal(norm);
	};

	//I remind you that the y component in the chunk origin is actually the world z component
	if (norm == glm::vec3(1.0f, 0.0f, 0.0f))
	{
		for (auto& block : chunk_blocks)
			if (block.position.x == s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::PlusX);
	}
	else if (norm == glm::vec3(-1.0f, 0.0f, 0.0f))
	{
		for (auto& block : chunk_blocks)
			if (block.position.x == 0)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::MinusX);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, 1.0f))
	{
		for (auto& block : chunk_blocks)
			if (block.position.z == s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::PlusZ);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, -1.0f))
	{
		for (auto& block : chunk_blocks)
			if (block.position.z == 0)
				erase_flanked_normal(block, norm, Defs::ChunkLocation::MinusZ);
	}

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

	MC_ASSERT(false, "Unreachable");
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
	for (u64 i = 0; i < chunk_blocks.size(); ++i)
	{
		auto& block = chunk_blocks[i];
		if (!depth_buf_draw)
		{
			//For standard drawcalls, we care about visible blocks
			if (!block.IsDrawable())
				continue;

			texindex_buf[count] = static_cast<u32>(block.Type());
			position_buf[count++] = ToWorld(block.position);

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

			position_buf[count++] = ToWorld(block.position);

			if (count == GlCore::g_MaxRenderedObjCount)
				GlCore::DispatchDepthRendering(position_buf, count);
		}
	}
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
		auto local_iter = std::find_if(chunk_blocks.begin(), chunk_blocks.end(),
			[neighbor_pos, this](const Block& b) {return ToWorld(b.position) == neighbor_pos; });

		//Behaviour between chunks
		if (local_iter == chunk_blocks.end())
		{
			if (!side_chunk_check)
			{
				if (m_PlusX.has_value() && norm == glm::vec3(1.0f, 0.0f, 0.0f))
					m_RelativeWorld.GetChunk(m_PlusX.value())->AddNewExposedNormals(pos, true);
				if (m_MinusX.has_value() && norm == glm::vec3(-1.0f, 0.0f, 0.0f))
					m_RelativeWorld.GetChunk(m_MinusX.value())->AddNewExposedNormals(pos, true);
				if (m_PlusZ.has_value() && norm == glm::vec3(0.0f, 0.0f, 1.0f))
					m_RelativeWorld.GetChunk(m_PlusZ.value())->AddNewExposedNormals(pos, true);
				if (m_MinusZ.has_value() && norm == glm::vec3(0.0f, 0.0f, -1.0f))
					m_RelativeWorld.GetChunk(m_MinusZ.value())->AddNewExposedNormals(pos, true);
			}
		}
		else
		{
			local_iter->AddNormal(-norm);
		}
	};

	compute_new_normals(block_pos, glm::vec3( 1.0f, 0.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(-1.0f, 0.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3( 0.0f, 1.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, -1.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3( 0.0f, 0.0f, 1.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, 0.0f, -1.0f));
}

void Chunk::AddWaterLayerIfPresent(glm::vec3* buffer, u32& count)
{
	if (!m_WaterLayerPositions.empty()) {
		std::memcpy(buffer + count, m_WaterLayerPositions.data(), m_WaterLayerPositions.size() * sizeof(glm::vec3));
		count += m_WaterLayerPositions.size();
	}
}

u32 Chunk::LastSelectedBlock() const
{
	return m_SelectedBlock;
}

u32 Chunk::SectorIndex() const
{
	return m_SectorIndex;
}

void Chunk::EmplaceLowerBlockStack(u16 stack_count)
{
	//Emplaces a new stack of some blocks
	s16 block_stack_size = 7;
	auto& generations = m_RelativeWorld.perlin_generations;
	u64 chunk_hash = Hash(glm::vec2(m_ChunkOrigin.x, m_ChunkOrigin.z));
	for (u16 stacks_count = 0; stacks_count < stack_count; stacks_count++) {
		//At the moment, do not create blocks below 0
		s16 actual_size = lower_threshold >= 7 ? block_stack_size : static_cast<s16>(lower_threshold);
		if (actual_size == 0)
			return;

		MC_ASSERT(generations.find(chunk_hash) != generations.end(), "This generation should exist");


		auto iter = chunk_blocks.begin();
		for (u32 x = 0; x < s_ChunkWidthAndHeight; x++) {
			for (u32 z = 0; z < s_ChunkWidthAndHeight; z++) {
				for (s16 y = static_cast<s16>(lower_threshold - 1); y >= static_cast<s16>(lower_threshold - actual_size); y--) {
					Defs::Item type = generations[chunk_hash].biomes[x * s_ChunkWidthAndHeight + z] == Defs::Biome::Plains ?
						Defs::Item::Stone :
						Defs::Item::Sand;

					iter = chunk_blocks.emplace(iter, glm::u8vec3(x, y, z), type);
				}

				while (iter != chunk_blocks.end() && iter->position.x == x && iter->position.z == z)
					iter++;
			}
		}

		lower_threshold -= actual_size;
	}
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
	sz& chunk_blocks.size();
	sz& m_WaterLayerPositions.size();

	sz& m_ChunkIndex;
	//World address does not need to be serialized

	if (!chunk_blocks.empty())
	{
		const glm::vec3& base_vec = m_ChunkOrigin;
		//We serialize only a single 12bytes position vector,
		//so the other can be stored as 3bytes vector offsets
		sz& base_vec.x& base_vec.y& base_vec.z;

		for (auto& block : chunk_blocks)
			block.Serialize(sz);
	}

	//Serialize water layers
	for (auto& layer : m_WaterLayerPositions)
		sz& layer.x& layer.y& layer.z;

	sz& m_ChunkOrigin.x & m_ChunkOrigin.z;
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
	u64 blk_vec_size;
	u64 water_layer_size;
	glm::vec3 base_vec;
	u32 adj_chunks[4];

	sz% blk_vec_size;
	sz% water_layer_size;

	sz% m_ChunkIndex;

	if (blk_vec_size != 0)
	{
		chunk_blocks.clear();

		sz% base_vec.x% base_vec.y% base_vec.z;

		glm::u8vec3 v;
		u8 norm_payload, item_type;

		for (u32 i = 0; i < blk_vec_size; i++)
		{
			sz% v.x% v.y% v.z;
			sz% norm_payload;
			sz% item_type;

			chunk_blocks.emplace_back(v, static_cast<Defs::Item>(item_type));
			auto& bb = chunk_blocks.back();
			bb.exposed_normals = norm_payload;
		}
	}

	//Deserialize water layers
	for (u32 i = 0; i < water_layer_size; i++)
	{
		f32 x, y, z;
		sz% x; sz% y; sz% z;
		m_WaterLayerPositions.emplace_back(x, y, z);
	}

	sz% m_ChunkOrigin.x% m_ChunkOrigin.z;
	m_ChunkOrigin.y = 0.0f;

	//Calculate chunk center
	m_ChunkCenter = m_ChunkOrigin + GetHalfWayVector();

	sz% adj_chunks[0] % adj_chunks[1] % adj_chunks[2] % adj_chunks[3];

	m_PlusX = adj_chunks[0] != -1 ? std::make_optional(adj_chunks[0]) : std::nullopt;
	m_MinusX = adj_chunks[1] != -1 ? std::make_optional(adj_chunks[1]) : std::nullopt;
	m_PlusZ = adj_chunks[2] != -1 ? std::make_optional(adj_chunks[2]) : std::nullopt;
	m_MinusZ = adj_chunks[3] != -1 ? std::make_optional(adj_chunks[3]) : std::nullopt;

	return sz;
}

bool Chunk::IsBlock(const glm::vec3& pos, s32 starting_index, bool search_towards_end, u32* block_index) const
{
	MC_ASSERT(starting_index >= 0 && starting_index < chunk_blocks.size(), "provided index out of bounds");

	if (search_towards_end)
	{
		//Searching before in the defined batch
		for (s32 i = starting_index; i < chunk_blocks.size(); i++)
		{
			if (ToWorld(chunk_blocks[i].position) == pos)
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
			if (ToWorld(chunk_blocks[i].position) == pos)
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
	return chunk_blocks[index];
}

const Block& Chunk::GetBlock(u32 index) const
{
	return chunk_blocks[index];
}

bool Chunk::BorderCheck(Chunk* chunk, const glm::vec3& pos, u32 top_index, u32 bot_index, bool search_dir)
{
	bool local_found = false, adjacent_found = false;
	u32 blk_index;
	local_found = IsBlock(ToWorld(chunk_blocks[top_index].position) + pos, bot_index, search_dir, &blk_index);
	if (!local_found)
			adjacent_found = chunk && chunk->IsBlock(ToWorld(chunk_blocks[top_index].position) + pos, 0, true, &blk_index);

	if (local_found) {
		if (chunk_blocks[blk_index].Type() != Defs::Item::Leaves)
			return true;
	}
	else if (adjacent_found) {
		if (chunk->GetBlock(blk_index).Type() != Defs::Item::Leaves)
			return true;
	}
	else {
		chunk_blocks[top_index].AddNormal(pos);
	}

	return false;
}

u64 Chunk::Hash(glm::vec2 v) 
{
	u64 ret;
	std::memcpy(&ret, &v, sizeof(u64));
	return ret;
};

