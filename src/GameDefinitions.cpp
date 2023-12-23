#include "GameDefinitions.h"
#include "State.h"
#include "Utils.h"

namespace Defs
{
    std::atomic<ViewMode> g_ViewMode = ViewMode::WorldInteraction;

    u16 g_InventoryKey = GLFW_KEY_R;
    MovementType g_MovementType = MovementType::Normal;

    u32 g_ScreenWidth = 0; 
    u32 g_ScreenHeight = 0;
    glm::vec3 g_PlayerAxisMapping = glm::vec3(1.0f);
    f32 g_PlayerSpeed = 0.0f;
	const f32 g_ChunkSpawningDistance = 500.0f;
	const f32 g_ChunkRenderingDistance = 300.0f;
	const f32 g_CameraCompensation = 10.0f;
	const f32 g_RenderDistance = 1500.0f;
    const f32 g_SectionDimension = 512.0f;
    u32 g_ChunkProgIndex = 0;
	const s32 g_SpawnerBegin = -64;
	const s32 g_SpawnerEnd = 64;
	const s32 g_SpawnerIncrement = 16;
    const glm::vec3 g_LightDirection{ 0.0f, -1.0f, 0.0f };
    std::atomic<u32> g_SelectedBlock{static_cast<u32>(-1)};
    std::atomic<u32> g_SelectedChunk{static_cast<u32>(-1)};
    Defs::Item g_InventorySelectedBlock = Defs::Item::Dirt;
    bool g_EnvironmentChange = false;
    //Shorthand for Sector SerialiZeD
    std::string g_SerializedFileFormat = ".sszd";
    std::unordered_set<u32> g_PushedSections;
    f32 water_limit = -0.25f;
    std::pair<f32, bool> jump_data = std::make_pair(0.0f, false);
    
    //Local variables for now
    f32 jump_height = 6.0f;

    void ReadWasd(const Window& window, bool& w, bool& a, bool& s, bool& d)
    {
        w = window.IsKeyboardEvent({ GLFW_KEY_W, GLFW_PRESS });
        a = window.IsKeyboardEvent({ GLFW_KEY_A, GLFW_PRESS });
        s = window.IsKeyboardEvent({ GLFW_KEY_S, GLFW_PRESS });
        d = window.IsKeyboardEvent({ GLFW_KEY_D, GLFW_PRESS });
    }

    void KeyboardFunction(const Window& window, Camera* camera, f64 elapsed_time)
    {
        f32 max_speed = 12.0f;
        f32 speed_increment_factor = 100.0f;

        //Give a starting point to every null coord if necessary
        bool mw, ma, ms, md;
        ReadWasd(window, mw, ma, ms, md);

        glm::vec3 front = camera->GetFront();
        if (g_MovementType != MovementType::Creative) {
            front.y = 0.0f;
        }

        if (mw)
            camera->position += front * g_PlayerSpeed * g_PlayerAxisMapping * (f32)elapsed_time;
        if (ms)
            camera->position += front * g_PlayerSpeed * g_PlayerAxisMapping * -(f32)elapsed_time;
        if (ma)
            camera->position += glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * g_PlayerSpeed * g_PlayerAxisMapping * -(f32)elapsed_time;
        if (md)
            camera->position += glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * g_PlayerSpeed * g_PlayerAxisMapping * (f32)elapsed_time;

        if (g_MovementType == MovementType::Creative) {
            if (window.IsKeyboardEvent({ GLFW_KEY_E, GLFW_PRESS }))
                camera->position += glm::vec3(0.0f, 1.0f, 0.0f) * g_PlayerSpeed * g_PlayerAxisMapping * (f32)elapsed_time;
            if (window.IsKeyboardEvent({ GLFW_KEY_C, GLFW_PRESS }))
                camera->position += glm::vec3(0.0f, 1.0f, 0.0f) * g_PlayerSpeed * g_PlayerAxisMapping * -(f32)elapsed_time;
        }
        else {
            if (window.IsKeyboardEvent({ GLFW_KEY_SPACE, GLFW_PRESS })) {
                if (jump_data.second) {
                    jump_data = { jump_height, false };
                }
            }
        }

        //If the player stops, then reduce the acceleration
        if (mw || ma || ms || md) {
            if (g_PlayerSpeed < max_speed)
                g_PlayerSpeed += speed_increment_factor * elapsed_time;
        }
        else {
            //Player did not move
            g_PlayerSpeed = 0.0f;
        }

#ifndef MC_STANDALONE
    #ifdef _DEBUG
            //DEBUG issue key
            if (window.IsKeyboardEvent({ GLFW_KEY_P, GLFW_PRESS }))
                 __debugbreak();
    #endif
#endif
    }

    void MouseFunction(const Window &window, Camera *camera, f64 x, f64 y, f64 dpi, f64 time)
    {
        double localx, localy;
        window.GetCursorCoord(localx, localy);

        camera->RotateX(static_cast<f32>((localx - x) * dpi * time));
        camera->RotateY(static_cast<f32>((y - localy) * dpi * time));
    }

    HitDirection ViewBlockCollision(const glm::vec3 &camera_pos, const glm::vec3 &camera_dir, const glm::vec3 &block_pos,
                            f32 &dist) 	{
        //Calculating distance
        dist = glm::length(camera_pos - block_pos);
        if (dist > 10.0f)
            return HitDirection::None;

        f32 half_cube_diag = 0.5f;
        glm::vec3 cube_min = block_pos - glm::vec3(half_cube_diag);
        glm::vec3 cube_max = block_pos + glm::vec3(half_cube_diag);

        f32 t0x = (cube_min.x - camera_pos.x) / camera_dir.x;
        f32 t1x = (cube_max.x - camera_pos.x) / camera_dir.x;

        if (t0x > t1x)
            std::swap(t0x, t1x);

        f32 t0y = (cube_min.y - camera_pos.y) / camera_dir.y;
        f32 t1y = (cube_max.y - camera_pos.y) / camera_dir.y;

        if (t0y > t1y)
            std::swap(t0y, t1y);

        if (t1x < t0y || t1y < t0x)
            return HitDirection::None;

        f32 t0z = (cube_min.z - camera_pos.z) / camera_dir.z;
        f32 t1z = (cube_max.z - camera_pos.z) / camera_dir.z;

        if (t0z > t1z)
            std::swap(t0z, t1z);

        f32 tc0 = std::max(t0x, t0y);
        f32 tc1 = std::min(t1x, t1y);

        if (t0z > tc1 || t1z < tc0)
            return HitDirection::None;

        glm::vec3 hitpoint = camera_pos + camera_dir * std::max(tc0, t0z);
        glm::vec3 dir = hitpoint - block_pos;
        glm::vec3 abs_dir = glm::abs(dir);

        static auto max3 = [](f32 f1, f32 f2, f32 f3) {
            if (f1 > f2 && f1 > f3)
                return 1;
            else if (f2 > f1 && f2 > f3)
                return 2;
            else return 3;
        };

        switch (max3(abs_dir.x, abs_dir.y, abs_dir.z))
        {
        case 1:
            if (dir.x > 0.0f)
                return HitDirection::PosX;
            else
                return HitDirection::NegX;
        case 2:
            if (dir.y > 0.0f)
                return HitDirection::PosY;
            else
                return HitDirection::NegY;
        case 3:
            if (dir.z > 0.0f)
                return HitDirection::PosZ;
            else
                return HitDirection::NegZ;
        }

        return HitDirection::None;
    }

    u32 ChunkSectorIndex(const glm::vec2& pos)
    {
        //Determine the chunk's level of distance
        s16 dist[2];
        dist[0] = static_cast<s16>(glm::floor(pos.x / g_SectionDimension));
        dist[1] = static_cast<s16>(glm::floor(pos.y / g_SectionDimension));

        u32 ret = 0;
        std::memcpy(&ret, dist, sizeof(u32));
        return ret;
    }

    //Addresses that map to some specific arrays used in WaterRegionLevel
    //do not need to be freed specifically
    glm::vec2* directions = nullptr;
    glm::vec2* compass_directions = nullptr;

    f32 WaterRegionLevel(f32 sx, f32 sy, const WorldSeed& seed, Utils::Vector<WaterArea>& pushed_areas)
    {

        static f32 watermap_unit = 1.0f / watermap_density;
        
        for (auto& area : pushed_areas)
            if (area.Contains(sx, sy))
                return area.water_height;


        const u32 directions_count = 9, compass_directions_count = 4;
        if (directions == nullptr || compass_directions == nullptr) {
            glm::vec2 directions_data[directions_count] =
            {
                glm::vec2(1.0f, -1.0f),glm::vec2(1.0f, 0.0f),glm::vec2(1.0f, 1.0f),glm::vec2(0.0f, 1.0f),
                glm::vec2(-1.0f, 1.0f),glm::vec2(-1.0f, 0.0f),glm::vec2(-1.0f, -1.0f),glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -1.0f)
            };
            glm::vec2 compass_direction_data[compass_directions_count] = { glm::vec2(1.0f, 0.0f), glm::vec2(-1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, -1.0f) };

            Memory::Arena* ma = GlCore::pstate->memory_arena;
            directions = static_cast<glm::vec2*>(Memory::AllocateUnchecked(ma, directions_count * sizeof(glm::vec2)));
            std::memcpy(directions, directions_data, sizeof(glm::vec2) * directions_count);
            compass_directions = static_cast<glm::vec2*>(Memory::AllocateUnchecked(ma, compass_directions_count * sizeof(glm::vec2)));
            std::memcpy(compass_directions, compass_direction_data, sizeof(glm::vec2) * compass_directions_count);
            ma->unfreed_mem += sizeof(directions_data) + sizeof(compass_direction_data);
        }

        //check for extra borders(should be rarely called)
        auto check_extra = [&](const glm::vec2& vec)
        {
            for (u32 i = 0; i < compass_directions_count; i++)
            {
                glm::vec2 pos = vec + compass_directions[i] * watermap_unit;
                f32 map_val = PerlNoise::GenerateSingleNoise(pos.x / watermap_density, pos.y / watermap_density, seed.secundary_seeds[0]);

                if (map_val > water_limit)
                    return true;
            }

            return false;
        };

        WaterArea wa;
        glm::vec2 prev_selection{ 0 };
        glm::vec2 curr_selection{ sx,sy };
        u32 iterations = 0;

        wa.pos_xz = curr_selection;
        wa.neg_xz = curr_selection;
        wa.water_height = INFINITY;

        
        //Search closest shoreline
        glm::vec2 search_vec{ 1.0f };
        f32 f0 = PerlNoise::GenerateSingleNoise(sx / landmap_density, sy / landmap_density, seed.seed_value);
        f32 f1 = PerlNoise::GenerateSingleNoise((sx + 10.0f) / landmap_density, sy / landmap_density, seed.seed_value);
        f32 f2 = PerlNoise::GenerateSingleNoise(sx / landmap_density, (sy + 10.0f) / landmap_density, seed.seed_value);

        if (f0 > f1)
            search_vec.x = -1.0f;

        if (f2 > f0)
            search_vec.y = -1.0f;

        bool alt = true;
        f32 lx = sx, ly = sy;
        while (PerlNoise::GenerateSingleNoise(lx / watermap_density, ly / watermap_density, seed.secundary_seeds[0]) < water_limit)
        {
            if(alt)
                lx += search_vec.x;
            else
                ly += search_vec.y;

            alt = !alt;
        }

        //variable setup
        glm::vec2 static_selection{ lx,ly };
        curr_selection = {lx, ly};

        //At the end of the coast scanning process, the selected coordinates
        //may be a little off, so we check if the selection is near enough to the
        //start after a good number of iterations
        while (!(iterations > 10 && glm::length(static_selection - curr_selection) < 5.0f))
        {
            bool found = false;
            bool good_border = false;
            //Start search from region 0
            for (u32 i = 0; i < directions_count; i++)
            {
                glm::vec2 pos = curr_selection + directions[i];
                if (pos == prev_selection)
                    continue;

                //We could GetBlockAltitude already here, but by retrieving only the water level and
                //calling that function anly when we have hit a new valid block saves us some performances

                //load water map value
                f32 map_val = PerlNoise::GenerateSingleNoise(pos.x / watermap_density, pos.y / watermap_density, seed.secundary_seeds[0]);

                if (map_val > water_limit)
                    good_border = true;
                else
                {
                    //this flag tells us that if we meet an underwater tile region we can skip
                    //the border check, because during the previous iteration we scanned a surface
                    //block, so the condition is met automatically
                    if (good_border)
                        found = true;
                    else
                        if (check_extra(pos))
                            found = true;
                }

                if (found)
                {
                    prev_selection = curr_selection;
                    curr_selection = pos;

                    //Need to load the final terrain height, already parsed with exponentials and 
                    //logarithms
                    f32 cur_val = PerlNoise::GetBlockAltitude(pos.x, pos.y, seed).altitude;
                    if (cur_val < wa.water_height)
                        wa.water_height = cur_val;

                    //Adjust water area
                    if (pos.x < wa.neg_xz.x)
                        wa.neg_xz.x = pos.x;
                    if(pos.y < wa.neg_xz.y)
                        wa.neg_xz.y = pos.y;
                    if (pos.x > wa.pos_xz.x)
                        wa.pos_xz.x = pos.x;
                    if (pos.y > wa.pos_xz.y)
                        wa.pos_xz.y = pos.y;

                    break;
                }
            }

            iterations++;
        }

        pushed_areas.push_back(wa);
        //This is done so that if there are smaller ponds which overlap with a bigger lake region, those ones have a bigger priority
        std::sort(pushed_areas.begin(), pushed_areas.end(), [](const WaterArea& a1, const WaterArea& a2) {return a1.Length() < a2.Length(); });
        return wa.water_height;
    }

    Utils::Vector<glm::vec3> GenerateRandomFoliage(Utils::Vector<glm::vec3>& possible_positions, std::mt19937& rand_engine)
    {
        Utils::Vector<u32> selected_indices;
        Utils::Vector<glm::vec3> ret;
        for (u32 i = 0; i < 14; i++) {
            u32 index;
            do {
                index = rand_engine() % possible_positions.size();
            } while (std::find(selected_indices.begin(), selected_indices.end(), index) != selected_indices.end());

            glm::vec3 selected = possible_positions[index];
            ret.push_back(selected);

            auto push_next_leafblock = [&](u32 index) {
                if (selected[index] == 0.0f)
                    return;

                glm::vec3 new_vec;
                
                switch (index) {
                case 0:
                    new_vec = selected + glm::vec3(selected[index] > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
                    break;
                case 1:
                    new_vec = selected + glm::vec3(0.0f, selected[index] > 0.0f ? -1.0f : 1.0f, 0.0f);
                    break;
                case 2:
                    new_vec = selected + glm::vec3(0.0f, 0.0f, selected[index] > 0.0f ? -1.0f : 1.0f);
                    break;
                }
                
                //Discard also the -1 y vector at the end
                if (std::find(ret.begin(), ret.end(), new_vec) == ret.end() && new_vec != glm::vec3(0.0f, -1.0f, 0.0f)) 
                    ret.push_back(new_vec);

                selected = new_vec;
            };

            //Algorithm which connects the current block to the tree base
            while (selected != glm::vec3(0.0f)) {
                //Select a random direction to follow
                
                push_next_leafblock(rand_engine() % 3);
            }

            selected_indices.push_back(index);
        }

        return ret;
    }

    

    namespace PerlNoise
    {
        //Init the seed with premultiplied constants,
        //So multiplying the seed to a random value to emulate
        //randomness is done only once
        void InitSeedMap(WorldSeed& seed)
        {
            if (seed.secundary_seeds.empty())
            {
                seed.secundary_seeds.resize(2);
                seed.secundary_seeds[0] = 28584983 * seed.seed_value;
                seed.secundary_seeds[1] = 3142523 * seed.seed_value;
            }
        }
        f32 Interpolate(f32 a0, f32 a1, f32 w)
        {
            return (a1 - a0) * w + a0;
        }
        glm::vec2 GenRandomVecFrom(s32 n1, s32 n2, const u64& seed)
        {
            // Snippet of code i took from the internet. scrambles
            //some values and spits out a reasonable random value
            const u32 w = 8 * sizeof(u32);
            const u32 s = w / 2; // rotation width
            u32 a = n1, b = n2;
            a *= 3284157443 * seed; b ^= a << s | a >> w - s;
            b *= 1911520717; a ^= b << s | b >> w - s;
            a *= 2048419325;
            f32 random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
            return { glm::cos(random), glm::sin(random) };
        }
        f32 PerformDot(s32 a, s32 b, f32 x, f32 y, const u64& seed) {
            glm::vec2 rand_vec = GenRandomVecFrom(a, b, seed);
            //Offset vector
            glm::vec2 offset{x - static_cast<f32>(a), y - static_cast<f32>(b)};
            return glm::dot(offset, rand_vec);
        }
        f32 GenerateSingleNoise(f32 x, f32 y, const u64& seed) {
            s32 x0, x1, y0, y1;
            f32 sx, sy;
            x0 = std::floor(x);
            x1 = x0 + 1;
            y0 = std::floor(y);
            y1 = y0 + 1;

            sx = x - static_cast<f32>(x0);
            sy = y - static_cast<f32>(y0);

            f32 f1, f2, f3, f4, fr1, fr2;
            f1 = PerformDot(x0, y0, x, y, seed);
            f2 = PerformDot(x1, y0, x, y, seed);
            f3 = PerformDot(x0, y1, x, y, seed);
            f4 = PerformDot(x1, y1, x, y, seed);

            fr1 = Interpolate(f1, f2, sx);
            fr2 = Interpolate(f3, f4, sx);
            return Interpolate(fr1, fr2, sy);
        }
        Generation GetBlockAltitude(f32 x, f32 y, const WorldSeed& seed)
        {
            //Biome distribution
            f32 biome_map = GenerateSingleNoise(x / landmap_density, y / landmap_density, seed.seed_value);
            Biome local_biome = biome_map < -0.2f ? Biome::Desert : Biome::Plains;

            f32 water_map = GenerateSingleNoise(x / watermap_density, y / watermap_density, seed.secundary_seeds[0]);

            //Terrain generation
            f32 fx = GenerateSingleNoise(x / 16.0f, y / 16.0f, seed.seed_value);
            f32 fy = GenerateSingleNoise(x / 40.0f, y / 40.0f, seed.secundary_seeds[0]);
            f32 fz = GenerateSingleNoise(x / 80.0f, y / 80.0f, seed.secundary_seeds[1]) * 3.0f;
            f32 terrain_output = fx + fy + fz;
            //Smoothing the landscape's slopes as we approach the desert
            if (biome_map < 0.0f)
                terrain_output *= glm::max(glm::exp(biome_map * 6.0f), 0.1f);

            if (water_map < water_limit)
            {
                terrain_output -= glm::log(1.0f + std::abs(water_map) + water_limit) * 23.0f;
                if (terrain_output < -4.0f)
                    terrain_output = -4.0f;
            }

            return { terrain_output, local_biome, water_map < water_limit };
        }
    }
}

namespace Physics {
    void HandlePlayerMovement(f32 elapsed_time) 
    {
        Camera& camera = *GlCore::pstate->camera;
        Window& window = *GlCore::pstate->game_window;
        camera.ProcessInput(window, elapsed_time, 0.8);
    }
    void HandlePlayerGravity(f32 elapsed_time) 
    {
        using Defs::jump_data;
        Camera& camera = *GlCore::pstate->camera;
        //Read the value first, as that may be modified from the upload thread(WARNING, might be better to add a mutex)
        f32 clamped_y = Defs::g_PlayerAxisMapping.y;

        jump_data.first -= clamped_y * 50.0f * elapsed_time;
        if (jump_data.first < -4.0f)
            jump_data.first = -4.0f;

        //Keep the 0.0f as it interrupts the player upon hitting blocks, but remove the exponential rise until 0.12
        //this makes the jump_data more sudden
        f32 directional_boost = clamped_y > 0.0f && clamped_y < 0.4f ? 0.4f : clamped_y;
        camera.position.y += jump_data.first * directional_boost * elapsed_time * 4.0f;
    }
    void ProcessPlayerAxisMovement(f32 elapsed_time)
    {
        f32 multiplier = 5.0f;
        f32 final_threshold = 1.0f;
        for (u32 i = 0; i < 3; i++) {
            f32& cd = Defs::g_PlayerAxisMapping[i];
            //accelerated increase in this axis speed
            if (cd >= final_threshold) {
                cd = final_threshold;
            }
            cd += multiplier * elapsed_time;
        }
    }
}
