#include "GameDefinitions.h"

namespace Gd
{
	const float g_ChunkSpawningDistance = 250.0f;
	const float g_ChunkRenderingDistance = 200.0f;
	const float g_CameraCompensation = 5.0f;
	const float g_RenderDistance = 1500.0f;
    const float g_FramedPlayerSpeed = 60.0f;
	const int32_t g_SpawnerBegin = -64;
	const int32_t g_SpawnerEnd = 64;
	const int32_t g_SpawnerIncrement = 16;

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

    bool ViewBlockCollision(const glm::vec3 &camera_pos, const glm::vec3 &camera_dir, const glm::vec3 &block_pos,
                            float &dist) 	{
        //Calculating distance
        dist = glm::length(camera_pos - block_pos);
        if (dist > 10.0f)
            return false;

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
            return false;

        float t0z = (cube_min.z - camera_pos.z) / camera_dir.z;
        float t1z = (cube_max.z - camera_pos.z) / camera_dir.z;

        if (t0z > t1z)
            std::swap(t0z, t1z);

        float tc0 = std::max(t0x, t0y);
        float tc1 = std::min(t1x, t1y);

        if (t0z > tc1 || t1z < tc0)
            return false;

        return true;
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
        float GetBlockAltitude(float x, float y, const WorldSeed& seed)
        {
            //Biome distribution TODO

            //Terrain generation
            float fx = GenerateSingleNoise(x / 16.0f, y / 16.0f, seed.seed_value);
            float fy = GenerateSingleNoise(x / 40.0f, y / 40.0f, seed.secundary_seeds[0]);
            float fz = GenerateSingleNoise(x / 60.0f, y / 60.0f, seed.secundary_seeds[1]) * 2.5f;
            return fx + fy + fz;
        }
    }
}
