//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "GameDefinitions.h"
#include "Vertices.h"
#include "State.h"

class Block;
class World;

namespace GlCore 
{
    //OpenGL loading/handling functions
    void LoadResources();
    void UpdateCamera();
    void RenderSkybox();
    //Renders the items that is held by the player
    void RenderHeldItem(Defs::Item sprite);
    void RenderCrossaim();
    void UpdateShadowFramebuffer();

    void UniformProjMatrix();
    void UniformViewMatrix();
    void InitGameTextures(std::vector<Texture>& textures);
}