#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"
#include "MainIncl.h"

std::shared_ptr<Entity> GlCore::BlockStructure::m_EntityPtr = nullptr;
std::shared_ptr<Shader> GlCore::BlockStructure::m_ShaderPtr = nullptr;
std::vector<Texture> GlCore::BlockStructure::m_Textures = {};

namespace GlCore
{
    //World definitions
    WorldStructure::WorldStructure(Camera& world_camera)
        :m_GameCamera(world_camera)
    {
        
    }

    const Camera& WorldStructure::GetGameCamera() const
    {
        return m_GameCamera;
    }

    void WorldStructure::SetShaderCameraMVP(Shader& shd) const
    {
        shd.UniformMat4f(m_GameCamera.GetProjMatrix(), "proj");
        shd.UniformMat4f(m_GameCamera.GetViewMatrix(), "view");
        //Model matrix gets set when the model is being drawn
    }

    void WorldStructure::UpdateCamera()
    {
    }


    BlockStructure::BlockStructure()
    {
        //Just need to be loaded once, every block shares this properies
        if(m_EntityPtr.get() == nullptr)
            InitEntity();
        
        if (m_ShaderPtr.get() == nullptr)
            InitShader();

        if(m_Textures.empty())
            InitTextures();
    }

    const std::vector<Texture>& BlockStructure::GetBlockTextures() const
    {
        return m_Textures;
    }

    void BlockStructure::Draw(const glm::vec3& pos, const GameDefs::BlockType& bt) const
    {
        Texture* current_texture;

        switch (bt)
        {
        case GameDefs::BlockType::DIRT:
            current_texture = &m_Textures[0];
            break;
        default:
            throw std::runtime_error("Texture preset for this block not found!");
        }

        current_texture->Bind(0);
        m_ShaderPtr->Uniform1i(0, "diffuse_texture");
        
        m_EntityPtr->ResetPosition();
        m_EntityPtr->Translate(pos);
        m_EntityPtr->Draw(*m_ShaderPtr);
    }

    void BlockStructure::InitEntity()
    {
        Utils::VertexData cd = Utils::Cube(Utils::VertexProps::POS_AND_TEX_COORDS);
        VertexManager vm_ptr(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
        m_EntityPtr = std::make_shared<Entity>(std::move(vm_ptr));
    }

    void BlockStructure::InitShader()
    {
        m_ShaderPtr = std::make_shared<Shader>("assets/shaders/basic_cube.shader");
    }

    void BlockStructure::InitTextures()
    {
        m_Textures.emplace_back("assets/textures/dirt.png", false, TextureFilter::Nearest);
    }


}