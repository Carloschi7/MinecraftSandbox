#pragma once
#include <iostream>
#include "Chunk.h"

class World {
public:
    World(const Window& game_window);
    //Draws the number of chunks visible to the player
    void DrawRenderable() const;
    void UpdateScene();

private:
    std::vector<Chunk> m_Chunks;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
};

