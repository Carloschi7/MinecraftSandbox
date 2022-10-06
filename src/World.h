#pragma once
#include <iostream>
#include "Chunk.h"

class World {
public:
    World(Camera& world_cam);
    //Draws the number of chunks visible to the player
    void DrawRenderable() const;
    void UpdateScene();

private:
    Block m_Block;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
};

