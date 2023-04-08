//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include <memory>
#include "GameDefinitions.h"
#include "Vertices.h"
#include "Renderer.h"
#include "State.h"

class Block;
class World;

namespace GlCore 
{
    //OpenGL loading/handling functions
    void LoadResources();
    void UpdateCamera();
    void RenderSkybox();
    void RenderCrossaim();
    void RenderInventory();
    void UpdateShadowFramebuffer();

    void UniformProjMatrix();
    void UniformViewMatrix();
    const std::vector<Texture>& GameTextures();
}