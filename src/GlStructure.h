//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include <memory>
#include "MainIncl.h"
#include "GameDefinitions.h"

class Block;
class World;

namespace GlCore 
{
    class WorldStructure
    {
    public:
        WorldStructure(Camera& world_camera);
        void SetShaderCameraMVP(Shader& shd) const;
        void UpdateCamera();

        const Camera& GetGameCamera() const;
    private:
        Camera& m_GameCamera;
        friend class World;
    };


    class BlockStructure 
    {
    public:
        BlockStructure();

        const std::vector<Texture>& GetBlockTextures() const;
        void Draw(const glm::vec3& pos, const GameDefs::BlockType& bt) const;

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