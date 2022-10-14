//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include <memory>
#include "GameDefinitions.h"

class Block;
class World;

namespace GlCore 
{
    //Also handles the game window and other stuff which should technlically not be defined here.
    //For the time being, the project is very small, and up until a certain point there will
    //only be one world, so up until the project gets to be something bigger, we will be sticking
    //to this method
    class WorldStructure
    {
    public:
        WorldStructure(const Window& window);
        void UpdateCamera();
        void RenderSkybox() const;
        void RenderCrossaim() const;
        
        GameDefs::RenderData GetRenderFrameInfo() const;
        GameDefs::ChunkBlockLogicData GetChunkBlockLogicData() const;

        const Camera& GetGameCamera() const;
    private:
        //General definitions
        const Window& m_GameWindow;
        Camera m_GameCamera;

        //Cubemap Stuff
        static std::shared_ptr<CubeMap> m_CubemapPtr;
        static std::shared_ptr<Shader> m_CubemapShaderPtr;
        //Crossaim stuff
        static std::shared_ptr<Shader> m_CrossaimShaderPtr;
        static std::shared_ptr<VertexManager> m_CrossaimVmPtr;
        friend class World;
    };

    class ChunkStructure
    {
    public:
        ChunkStructure();
        void BlockRenderInit(const GameDefs::RenderData& rd, std::shared_ptr<Shader> block_shader) const;
    };

    class BlockStructure 
    {
    public:
        BlockStructure(const glm::vec3& pos, const GameDefs::BlockType& bt);
        void Draw(const std::vector<glm::vec3>& exp_norms, bool is_block_selected) const;
        
        const std::vector<Texture>& GetBlockTextures() const;
        std::shared_ptr<Shader> GetShader() const;

    private:
        //Handles the position of the block in the world;
        glm::mat4 m_ModelMatrix;
        //Texture of the current block
        Texture* m_CurrentTexture;
        VertexManager* m_CurrentVertexManager;

        //Cube geometry
        //The core OpenGL structures which is designed to do actual drawing/rendering
        static std::shared_ptr<VertexManager> m_VertexManagerSinglePtr;
        static std::shared_ptr<VertexManager> m_VertexManagerSidedPtr;
        //Instructions on how to draw the cube
        static std::shared_ptr<Shader> m_ShaderPtr;
        //Various cube textures
        static std::vector<Texture> m_Textures;
        friend class Block;
    };
}