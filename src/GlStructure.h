//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include <memory>
#include "GameDefinitions.h"

class Block;
class World;

namespace GlCore 
{
    class WorldStructure
    {
    public:
        WorldStructure(const Window& window);
        GameDefs::RenderData GetRenderFrameInfo() const;
        void UpdateCamera();
        void RenderCrossaim() const;

        const Camera& GetGameCamera() const;
    private:
        const Window& m_GameWindow;
        Camera m_GameCamera;

        static std::shared_ptr<Shader> m_CrossaimShaderPtr;
        VertexManager m_CrossaimVm;
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
        BlockStructure();

        const std::vector<Texture>& GetBlockTextures() const;
        void Draw(const glm::vec3& pos, const GameDefs::BlockType& bt, bool is_block_selected) const;

    private:
        void InitEntity();
        void InitShader();
        void InitTextures();

        //Cube geometry
        //The core OpenGL structures which is designed to do actual drawing/rendering
        static std::shared_ptr<Entity> m_EntityPtr;
        //Instructions on how to draw the cube
        static std::shared_ptr<Shader> m_ShaderPtr;
        //Various cube textures
        static std::vector<Texture> m_Textures;
        friend class Block;
    };
}