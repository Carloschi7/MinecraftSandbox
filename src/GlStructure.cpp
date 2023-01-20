#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"

std::shared_ptr<CubeMap> GlCore::WorldStructure::m_CubemapPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CubemapShaderPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CrossaimShaderPtr = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_CrossaimVmPtr = nullptr;

std::shared_ptr<VertexManager> GlCore::BlockStructure::m_VertexManagerPtr = nullptr;
std::shared_ptr<Shader> GlCore::BlockStructure::m_ShaderPtr = nullptr;
std::vector<Texture> GlCore::BlockStructure::m_Textures = {};

namespace GlCore
{
    //World definitions
    WorldStructure::WorldStructure()
    {
        Camera& cam = Root::GameCamera();
        cam.SetVectors(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

        cam.SetPerspectiveValues(glm::radians(45.0f),
            float(Root::GameWindow().Width()) / float(Root::GameWindow().Height()),
            0.1f,
            Gd::g_RenderDistance);

        cam.SetKeyboardFunction(Gd::KeyboardFunction);
        cam.SetMouseFunction(Gd::MouseFunction);
        
        //Loading static members
        if (m_CrossaimShaderPtr.get() == nullptr)
        {
            //Loading cubemap data
            std::vector<std::string> skybox_files =
            {
                "assets/textures/ShadedBackground.png",
                "assets/textures/ShadedBackground.png",
                "assets/textures/BotBackground.png",
                "assets/textures/TopBackground.png",
                "assets/textures/ShadedBackground.png",
                "assets/textures/ShadedBackground.png",
            };
            m_CubemapPtr = std::make_shared<CubeMap>(skybox_files, Gd::g_RenderDistance / 2.0f);

            m_CubemapShaderPtr = std::make_shared<Shader>("assets/shaders/cubemap.shader");
            m_CubemapShaderPtr->UniformMat4f(cam.GetProjMatrix(), g_ProjUniformName);

            //Loading crossaim data
            m_CrossaimShaderPtr = std::make_shared<Shader>("assets/shaders/basic_overlay.shader");
            //We set the crossaim model matrix here, for now this shader is used only 
            //for drawing this
            m_CrossaimShaderPtr->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), g_ModelUniformName);

            VertexData rd = CrossAim();
            //VM init only the first time
            m_CrossaimVmPtr = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        }
    }


    void WorldStructure::UpdateCamera()
    {
        Root::GameCamera().ProcessInput(Root::GameWindow(), 1.0f);
    }

    void WorldStructure::RenderSkybox() const
    {
        //SetViewMatrix with no translation
        glm::mat4 view = glm::mat4(glm::mat3(Root::GameCamera().GetViewMatrix()));
        RendererPayload pl{ view,
            &m_CubemapPtr->GetVertexManager(),
            m_CubemapShaderPtr.get(),
            nullptr,
            m_CubemapPtr.get(),
            nullptr,
            false};

        Renderer::Render(pl);
    }

    void WorldStructure::RenderCrossaim() const
    {
        RendererPayload pl{ {}, m_CrossaimVmPtr.get(), m_CrossaimShaderPtr.get(), nullptr, nullptr, nullptr, false };
        Renderer::Render(pl);
    }

    void WorldStructure::UniformProjMatrix() const
    {
        auto block_shader = Root::BlockShader();
        block_shader->UniformMat4f(Root::GameCamera().GetProjMatrix(), g_ProjUniformName);
    }

    void WorldStructure::UniformViewMatrix() const
    {
        auto block_shader = Root::BlockShader();
        block_shader->UniformMat4f(Root::GameCamera().GetViewMatrix(), g_ViewUniformName);
    }


    BlockStructure::BlockStructure(const glm::vec3& pos, const Gd::BlockType& bt)
    {
        //Load static data
        if (m_VertexManagerPtr.get() != nullptr)
            return;
        
        VertexData cd = Cube();
        m_VertexManagerPtr = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);

        m_ShaderPtr = std::make_shared<Shader>("assets/shaders/basic_cube.shader");
        //Uniforming light direction(static for now)
        m_ShaderPtr->UniformVec3f(Gd::g_LightDirection, "light_direction");

        m_Textures.emplace_back("assets/textures/dirt.png", false, TextureFilter::Nearest);
        m_Textures.emplace_back("assets/textures/grass.png", false, TextureFilter::Nearest);
        m_Textures.emplace_back("assets/textures/sand.png", false, TextureFilter::Nearest);
        m_Textures.emplace_back("assets/textures/trunk.png", false, TextureFilter::Nearest);
        m_Textures.emplace_back("assets/textures/leaves.png", false, TextureFilter::Nearest);

        std::string tex_names[5]{
            "texture_dirt",
            "texture_grass",
            "texture_sand",
            "texture_trunk",
            "texture_leaves"
        };

        for (uint32_t i = 0; i < m_Textures.size(); i++)
        {
            m_Textures[i].Bind(i);
            m_ShaderPtr->Uniform1i(i, tex_names[i]);
        }

        //Create instance buffer for model matrices
        m_VertexManagerPtr->PushInstancedMatrixBuffer(nullptr, sizeof(glm::mat4) * g_MaxInstancedObjs,
            m_ShaderPtr->GetAttributeLocation("model_matrix"));

        LayoutElement el{ 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t), 0 };
        m_VertexManagerPtr->PushInstancedAttribute(nullptr, sizeof(uint32_t) * g_MaxInstancedObjs,
            m_ShaderPtr->GetAttributeLocation("tex_index"), el);

        //Root management
        Root::SetBlockShader(m_ShaderPtr);
        Root::SetBlockVM(m_VertexManagerPtr);
    }

    const std::vector<Texture>& BlockStructure::GetBlockTextures() const
    {
        return m_Textures;
    }

    void BlockStructure::Draw(const glm::vec3& pos, const Gd::BlockType& bt,
        const DrawableData& exp_norms, bool is_block_selected) const
    {
        Texture* current_texture = nullptr;
        switch (bt)
        {
        case Gd::BlockType::DIRT:
            current_texture = &m_Textures[0];
            break;
        case Gd::BlockType::GRASS:
            current_texture = &m_Textures[1];
            break;
        case Gd::BlockType::SAND:
            current_texture = &m_Textures[2];
            break;
        case Gd::BlockType::WOOD:
            current_texture = &m_Textures[3];
            break;
        case Gd::BlockType::LEAVES:
            current_texture = &m_Textures[4];
            break;
        default:
            throw std::runtime_error("Texture preset for this block not found!");
        }


        RendererPayload pl{ glm::translate(g_IdentityMatrix, pos),
                            m_VertexManagerPtr.get(),
                            m_ShaderPtr.get(),
                            current_texture,
                            nullptr,
                            &exp_norms,
                            is_block_selected };


        Renderer::Render(pl);
    }
}
