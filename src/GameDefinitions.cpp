#include "GameDefinitions.h"

namespace Gd
{
    std::atomic<ViewMode> g_GameMode = ViewMode::WorldInteraction;

	const float g_ChunkSpawningDistance = 500.0f;
	const float g_ChunkRenderingDistance = 300.0f;
	const float g_CameraCompensation = 10.0f;
	const float g_RenderDistance = 1500.0f;
    const float g_FramedPlayerSpeed = 80.0f;
    const float g_SectionDimension = 512.0f;
    uint32_t g_ChunkProgIndex = 0;
	const int32_t g_SpawnerBegin = -64;
	const int32_t g_SpawnerEnd = 64;
	const int32_t g_SpawnerIncrement = 16;
    const glm::vec3 g_LightDirection{ 0.0f, -1.0f, 0.0f };
    std::atomic_uint32_t g_SelectedBlock{static_cast<uint32_t>(-1)};
    std::atomic_uint32_t g_SelectedChunk{static_cast<uint32_t>(-1)};
    extern bool g_BlockDestroyed = false;
    //Shorthand for Sector SerialiZeD
    std::string g_SerializedFileFormat = ".sszd";
    std::unordered_set<uint32_t> g_PushedSections;

    float water_limit = -0.25f;

    void KeyboardFunction(const Window& window, Camera* camera, double time)
    {
        float fScalar = 0.6f;

        if (window.IsKeyboardEvent({ GLFW_KEY_W, GLFW_PRESS }))
            camera->MoveTowardsFront(fScalar * time);
        if (window.IsKeyboardEvent({ GLFW_KEY_S, GLFW_PRESS }))
            camera->MoveTowardsFront(-fScalar * time);
        if (window.IsKeyboardEvent({ GLFW_KEY_A, GLFW_PRESS }))
            camera->StrafeX(-fScalar * time);
        if (window.IsKeyboardEvent({ GLFW_KEY_D, GLFW_PRESS }))
            camera->StrafeX(fScalar * time);
        if (window.IsKeyboardEvent({ GLFW_KEY_E, GLFW_PRESS }))
            camera->StrafeY(fScalar * time);
        if (window.IsKeyboardEvent({ GLFW_KEY_C, GLFW_PRESS }))
            camera->StrafeY(-fScalar * time);
    }

    void MouseFunction(const Window &window, Camera *camera, double x, double y, double dpi, double time)
    {
        double localx, localy;
        window.GetCursorCoord(localx, localy);

        camera->RotateX((localx - x) * dpi * time);
        camera->RotateY((y - localy) * dpi * time);
    }

    HitDirection ViewBlockCollision(const glm::vec3 &camera_pos, const glm::vec3 &camera_dir, const glm::vec3 &block_pos,
                            float &dist) 	{
        //Calculating distance
        dist = glm::length(camera_pos - block_pos);
        if (dist > 10.0f)
            return HitDirection::None;

        static float half_cube_diag = 0.5f;
        glm::vec3 cube_min = block_pos - glm::vec3(half_cube_diag);
        glm::vec3 cube_max = block_pos + glm::vec3(half_cube_diag);

        float t0x = (cube_min.x - camera_pos.x) / camera_dir.x;
        float t1x = (cube_max.x - camera_pos.x) / camera_dir.x;

        if (t0x > t1x)
            std::swap(t0x, t1x);

        float t0y = (cube_min.y - camera_pos.y) / camera_dir.y;
        float t1y = (cube_max.y - camera_pos.y) / camera_dir.y;

        if (t0y > t1y)
            std::swap(t0y, t1y);

        if (t1x < t0y || t1y < t0x)
            return HitDirection::None;

        float t0z = (cube_min.z - camera_pos.z) / camera_dir.z;
        float t1z = (cube_max.z - camera_pos.z) / camera_dir.z;

        if (t0z > t1z)
            std::swap(t0z, t1z);

        float tc0 = std::max(t0x, t0y);
        float tc1 = std::min(t1x, t1y);

        if (t0z > tc1 || t1z < tc0)
            return HitDirection::None;

        glm::vec3 hitpoint = camera_pos + camera_dir * std::max(tc0, t0z);
        glm::vec3 dir = hitpoint - block_pos;
        glm::vec3 abs_dir = glm::abs(dir);

        static auto max3 = [](float f1, float f2, float f3) {
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

    uint32_t ChunkSectorIndex(const glm::vec2& pos)
    {
        //Determine the chunk's level of distance
        int16_t dist[2];
        dist[0] = static_cast<int16_t>(glm::floor(pos.x / g_SectionDimension));
        dist[1] = static_cast<int16_t>(glm::floor(pos.y / g_SectionDimension));

        uint32_t ret = 0;
        std::memcpy(&ret, dist, sizeof(uint32_t));
        return ret;
    }

    float WaterRegionLevel(float sx, float sy, const WorldSeed& seed)
    {
        static std::vector<WaterArea> pushed_areas;
        static float watermap_unit = 1.0f / watermap_density;
        
        for (auto& area : pushed_areas)
            if (area.Contains(sx, sy))
                return area.water_height;

        static std::vector<glm::vec2> directions =
        {
            glm::vec2(1.0f, -1.0f),glm::vec2(1.0f, 0.0f),glm::vec2(1.0f, 1.0f),glm::vec2(0.0f, 1.0f),
            glm::vec2(-1.0f, 1.0f),glm::vec2(-1.0f, 0.0f),glm::vec2(-1.0f, -1.0f),glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -1.0f)
        };

        //check for extra borders(should be rarely called)
        auto check_extra = [&](const glm::vec2& vec)
        {
            for (auto v : { glm::vec2(1.0f, 0.0f), glm::vec2(-1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, -1.0f) })
            {
                glm::vec2 pos = vec + v * watermap_unit;
                float map_val = PerlNoise::GenerateSingleNoise(pos.x / watermap_density, pos.y / watermap_density, seed.secundary_seeds[0]);

                if (map_val > water_limit)
                    return true;
            }

            return false;
        };

        WaterArea wa;
        glm::vec2 prev_selection{ 0 };
        glm::vec2 curr_selection{ sx,sy };
        uint32_t iterations = 0;

        wa.pos_xz = curr_selection;
        wa.neg_xz = curr_selection;
        wa.water_height = INFINITY;

        
        //Search closest shoreline
        glm::vec2 search_vec{ 1.0f };
        float f0 = PerlNoise::GenerateSingleNoise(sx / landmap_density, sy / landmap_density, seed.seed_value);
        float f1 = PerlNoise::GenerateSingleNoise((sx + 10.0f) / landmap_density, sy / landmap_density, seed.seed_value);
        float f2 = PerlNoise::GenerateSingleNoise(sx / landmap_density, (sy + 10.0f) / landmap_density, seed.seed_value);

        if (f0 > f1)
            search_vec.x = -1.0f;

        if (f2 > f0)
            search_vec.y = -1.0f;

        bool alt = true;
        float lx = sx, ly = sy;
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
            for (auto vec : directions)
            {
                glm::vec2 pos = curr_selection + vec;
                if (pos == prev_selection)
                    continue;

                //We could GetBlockAltitude already here, but by retrieving only the water level and
                //calling that function anly when we have hit a new valid block saves us some performances

                //load water map value
                float map_val = PerlNoise::GenerateSingleNoise(pos.x / watermap_density, pos.y / watermap_density, seed.secundary_seeds[0]);

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
                    float cur_val = PerlNoise::GetBlockAltitude(pos.x, pos.y, seed).altitude;
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
        float Interpolate(float a0, float a1, float w)
        {
            return (a1 - a0) * w + a0;
        }
        glm::vec2 GenRandomVecFrom(int32_t n1, int32_t n2, const uint64_t& seed)
        {
            // Snippet of code i took from the internet. scrambles
            //some values and spits out a reasonable random value
            const uint32_t w = 8 * sizeof(uint32_t);
            const uint32_t s = w / 2; // rotation width
            uint32_t a = n1, b = n2;
            a *= 3284157443 * seed; b ^= a << s | a >> w - s;
            b *= 1911520717; a ^= b << s | b >> w - s;
            a *= 2048419325;
            float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
            return { glm::cos(random), glm::sin(random) };
        }
        float PerformDot(int32_t a, int32_t b, float x, float y, const uint64_t& seed) {
            glm::vec2 rand_vec = GenRandomVecFrom(a, b, seed);
            //Offset vector
            glm::vec2 offset{x - static_cast<float>(a), y - static_cast<float>(b)};
            return glm::dot(offset, rand_vec);
        }
        float GenerateSingleNoise(float x, float y, const uint64_t& seed) {
            int32_t x0, x1, y0, y1;
            float sx, sy;
            x0 = std::floor(x);
            x1 = x0 + 1;
            y0 = std::floor(y);
            y1 = y0 + 1;

            sx = x - static_cast<float>(x0);
            sy = y - static_cast<float>(y0);

            float f1, f2, f3, f4, fr1, fr2;
            f1 = PerformDot(x0, y0, x, y, seed);
            f2 = PerformDot(x1, y0, x, y, seed);
            f3 = PerformDot(x0, y1, x, y, seed);
            f4 = PerformDot(x1, y1, x, y, seed);

            fr1 = Interpolate(f1, f2, sx);
            fr2 = Interpolate(f3, f4, sx);
            return Interpolate(fr1, fr2, sy);
        }
        Generation GetBlockAltitude(float x, float y, const WorldSeed& seed)
        {
            //Biome distribution
            float biome_map = GenerateSingleNoise(x / landmap_density, y / landmap_density, seed.seed_value);
            Biome local_biome = biome_map < -0.2f ? Biome::Desert : Biome::Plains;

            float water_map = GenerateSingleNoise(x / watermap_density, y / watermap_density, seed.secundary_seeds[0]);

            //Terrain generation
            float fx = GenerateSingleNoise(x / 16.0f, y / 16.0f, seed.seed_value);
            float fy = GenerateSingleNoise(x / 40.0f, y / 40.0f, seed.secundary_seeds[0]);
            float fz = GenerateSingleNoise(x / 80.0f, y / 80.0f, seed.secundary_seeds[1]) * 3.0f;
            float terrain_output = fx + fy + fz;
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
