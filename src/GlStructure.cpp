#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"
#include "Renderer.h"

std::shared_ptr<CubeMap> GlCore::WorldStructure::m_CubemapPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CubemapShaderPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CrossaimShaderPtr = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_CrossaimVmPtr = nullptr;

std::shared_ptr<VertexManager> GlCore::BlockStructure::m_VertexManagerSinglePtr = nullptr;
std::shared_ptr<VertexManager> GlCore::BlockStructure::m_VertexManagerSidedPtr = nullptr;
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
            GameDefs::g_RenderDistance);

        cam.SetKeyboardFunction(GameDefs::KeyboardFunction);
        cam.SetMouseFunction(GameDefs::MouseFunction);
        
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
            m_CubemapPtr = std::make_shared<CubeMap>(skybox_files, GameDefs::g_RenderDistance / 2.0f);

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

    GameDefs::RenderData WorldStructure::GetRenderFrameInfo()
    {
        GameDefs::RenderData rd;
        Camera& cam = Root::GameCamera();

        rd.camera_position = cam.GetPosition();
        rd.proj_matrix = cam.GetProjMatrix();
        rd.view_matrix = cam.GetViewMatrix();
        rd.p_key = Root::GameWindow().IsKeyboardEvent({ GLFW_KEY_P, GLFW_PRESS });
        return rd;
    }

    GameDefs::ChunkLogicData WorldStructure::GetChunkLogicData()
    {
        GameDefs::ChunkLogicData ld;
        ld.mouse_input.left_click = Root::GameWindow().IsMouseEvent({ GLFW_MOUSE_BUTTON_1, GLFW_PRESS });
        ld.mouse_input.right_click = Root::GameWindow().IsMouseEvent({ GLFW_MOUSE_BUTTON_2, GLFW_PRESS });
        ld.camera_position = Root::GameCamera().GetPosition();
        ld.camera_direction = Root::GameCamera().GetFront();
        return ld;
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
            false,
            false};

        Renderer::Render(pl);
    }

    void WorldStructure::RenderCrossaim() const
    {
        RendererPayload pl{ {}, m_CrossaimVmPtr.get(), m_CrossaimShaderPtr.get(), nullptr, nullptr, nullptr, false, true };
        Renderer::Render(pl);
    }

    void WorldStructure::UniformRenderInit(const GameDefs::RenderData& rd) const
    {
        auto block_shader = Root::BlockShader();
        block_shader->UniformMat4f(rd.proj_matrix, g_ProjUniformName);
        block_shader->UniformMat4f(rd.view_matrix, g_ViewUniformName);
    }

    ChunkStructure::ChunkStructure()
    {
    }


    BlockStructure::BlockStructure(const glm::vec3& pos, const GameDefs::BlockType& bt)
        :m_CurrentTexture(nullptr), m_CurrentVertexManager(nullptr), m_ModelPos(pos)
    {
        //Load static data
        if (m_VertexManagerSinglePtr.get() == nullptr)
        {
            VertexData cd = Cube(VertexProps::POS_AND_TEX_COORDS_SINGLE);
            m_VertexManagerSinglePtr = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
            cd = Cube(VertexProps::POS_AND_TEX_COORD_SIDED);
            m_VertexManagerSidedPtr = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);

            m_ShaderPtr = std::make_shared<Shader>("assets/shaders/basic_cube.shader");

            m_Textures.emplace_back("assets/textures/dirt.png", false, TextureFilter::Nearest);
            m_Textures.emplace_back("assets/textures/grass.png", false, TextureFilter::Nearest);

            //Root management
            Root::SetBlockShader(m_ShaderPtr);
        } 

        switch (bt)
        {
        case GameDefs::BlockType::DIRT:
            m_CurrentTexture = &m_Textures[0];
            m_CurrentVertexManager = m_VertexManagerSinglePtr.get();
            break;
        case GameDefs::BlockType::GRASS:
            m_CurrentTexture = &m_Textures[1];
            m_CurrentVertexManager = m_VertexManagerSidedPtr.get();
            break;
        default:
            throw std::runtime_error("Texture preset for this block not found!");
        }
    }

    const std::vector<Texture>& BlockStructure::GetBlockTextures() const
    {
        return m_Textures;
    }

    void BlockStructure::Draw(const DrawableData& exp_norms, bool is_block_selected) const
    {
        RendererPayload pl{ glm::translate(g_IdentityMatrix, m_ModelPos),
                            m_CurrentVertexManager,
                            m_ShaderPtr.get(),
                            m_CurrentTexture,
                            nullptr,
                            &exp_norms,
                            is_block_selected,
                            false};

        Renderer::Render(pl);
    }
}
