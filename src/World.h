#pragma once
#include <iostream>
#include "Chunk.h"

class World {
public:
    World(const Window& game_window);
    //Draws the number of chunks visible to the player
    void DrawRenderable() const;
    void UpdateScene();

    //Returns the corresponding chunk iterator if exists
    std::vector<Chunk>::iterator IsChunk(const Chunk& chunk, const GameDefs::ChunkLocation& cl);
    std::vector<Chunk>::iterator ChunkCend();

private:
    std::vector<Chunk> m_Chunks;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
};

