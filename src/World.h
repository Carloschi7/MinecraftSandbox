#pragma once
#include <iostream>
#include <optional>
#include "Chunk.h"

class World {
public:
    World(const Window& game_window);
    //Draws the number of chunks visible to the player
    void DrawRenderable() const;
    void UpdateScene();

    //Returns the corresponding chunk iterator if exists
    std::optional<uint32_t> IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl);
    Chunk& GetChunk(uint32_t index);

private:
    std::vector<Chunk> m_Chunks;
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    std::vector<glm::vec3> m_SpawnableChunks;
    //Last player pos
    glm::vec3 m_LastPos;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
};

