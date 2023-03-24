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
    //Also handles the game window and other stuff which should technically not be defined here.
    //For the time being, the project is very small, and up until a certain point there will
    //only be one world, so up until the project gets to be something bigger, we will be sticking
    //to this method
    class WorldStructure
    {
    public:
        WorldStructure();
        void UpdateCamera();
        void RenderSkybox() const;
        void RenderCrossaim() const;
        void UpdateShadowFramebuffer() const;

        void UniformProjMatrix() const;
        void UniformViewMatrix() const;
    private:
        State& m_State;
        //Cubemap Stuff
        static std::shared_ptr<CubeMap> m_CubemapPtr;
        static std::shared_ptr<Shader> m_CubemapShaderPtr;
        //Crossaim stuff
        static std::shared_ptr<Shader> m_CrossaimShaderPtr;
        static std::shared_ptr<VertexManager> m_CrossaimVmPtr;
        //Water stuff
        static std::shared_ptr<Shader> m_WaterShader;
        static std::shared_ptr<VertexManager> m_WaterVmPtr;
        //Shadow framebuffer
        static std::shared_ptr<FrameBuffer> m_DepthFramebufferPtr;
        static std::shared_ptr<Shader> m_FramebufferShaderPtr;
        static std::shared_ptr<VertexManager> m_DepthVMPtr;
        friend class World;
    };

    class BlockStructure 
    {
    public:
        BlockStructure(const glm::vec3& pos, const Gd::BlockType& bt);
    private:
        //Cube geometry
        //The core OpenGL structures which is designed to do actual drawing/rendering
        static std::shared_ptr<VertexManager> m_VertexManagerPtr;
        //Instructions on how to draw the cube
        static std::shared_ptr<Shader> m_ShaderPtr;
        friend class Block;
    };

    const std::vector<Texture>& LoadGameTextures();
}