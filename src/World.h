#pragma once
#include <iostream>
#include <optional>
#include <thread>
#include "Chunk.h"

class World {
public:
    World();
    ~World();
    //Draws the number of chunks visible to the player
    void DrawRenderable() const;
    void UpdateScene();

    //Returns the corresponding chunk index if exists
    std::optional<uint32_t> IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl);
    Chunk& GetChunk(uint32_t index);

private:
    //Function which handles spawnable chunk pushing conditions
    bool IsPushable(const Chunk& chunk, const GameDefs::ChunkLocation& cl, const glm::vec3& vec);
private:
    VecType<Chunk> m_Chunks;
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    std::vector<glm::vec3> m_SpawnableChunks;
    //Last player pos
    glm::vec3 m_LastPos;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
};

