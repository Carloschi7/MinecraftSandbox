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
    WorldStructure::WorldStructure(const Window& window)
        :m_GameWindow(window)
    {
        static constexpr float fRenderDistance = 1000.0f;
        m_GameCamera.SetVectors(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

        m_GameCamera.SetPerspectiveValues(glm::radians(45.0f),
            float(m_GameWindow.Width()) / float(m_GameWindow.Height()),
            0.1f,
            fRenderDistance);

        m_GameCamera.SetKeyboardFunction(GameDefs::KeyboardFunction);
        m_GameCamera.SetMouseFunction(GameDefs::MouseFunction);

        
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
            m_CubemapPtr = std::make_shared<CubeMap>(skybox_files, fRenderDistance / 2.0f);

            m_CubemapShaderPtr = std::make_shared<Shader>("assets/shaders/cubemap.shader");
            m_CubemapShaderPtr->UniformMat4f(m_GameCamera.GetProjMatrix(), g_ProjUniformName);

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

    GameDefs::RenderData WorldStructure::GetRenderFrameInfo() const
    {
        GameDefs::RenderData rd;
        rd.camera_position = m_GameCamera.GetPosition();
        rd.proj_matrix = m_GameCamera.GetProjMatrix();
        rd.view_matrix = m_GameCamera.GetViewMatrix();
        rd.p_key = m_GameWindow.IsKeyboardEvent({ GLFW_KEY_P, GLFW_PRESS });
        return rd;
    }

    GameDefs::ChunkLogicData WorldStructure::GetChunkLogicData() const
    {
        GameDefs::ChunkLogicData ld;
        ld.mouse_input.left_click = m_GameWindow.IsMouseEvent({ GLFW_MOUSE_BUTTON_1, GLFW_PRESS });
        ld.mouse_input.right_click = m_GameWindow.IsMouseEvent({ GLFW_MOUSE_BUTTON_2, GLFW_PRESS });
        ld.camera_position = m_GameCamera.GetPosition();
        ld.camera_direction = m_GameCamera.GetFront();
        return ld;
    }

    const Camera& WorldStructure::GetGameCamera() const
    {
        return m_GameCamera;
    }

    void WorldStructure::UpdateCamera()
    {
        m_GameCamera.ProcessInput(m_GameWindow, 1.0f);
    }

    void WorldStructure::RenderSkybox() const
    {
        m_CubemapPtr->BindTexture();
        m_CubemapShaderPtr->Uniform1i(0, g_SkyboxUniformName);

        //SetViewMatrix with no translation
        glm::mat4 view = glm::mat4(glm::mat3(m_GameCamera.GetViewMatrix()));
        m_CubemapShaderPtr->UniformMat4f(view, g_ViewUniformName);

        Renderer::Render({}, m_CubemapPtr->GetVertexManager(), *m_CubemapShaderPtr);
    }

    void WorldStructure::RenderCrossaim() const
    {
        Renderer::Render({}, *m_CrossaimVmPtr, *m_CrossaimShaderPtr);
    }

    void WorldStructure::UniformRenderInit(const GameDefs::RenderData& rd, std::shared_ptr<Shader> block_shader) const
    {
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

    std::shared_ptr<Shader> BlockStructure::GetShader()
    {
        return m_ShaderPtr;
    }

    void BlockStructure::Draw(const DrawableData& exp_norms, bool is_block_selected) const
    {
        if (is_block_selected)
            m_ShaderPtr->Uniform1i(true, g_EntitySelectedUniformName);

        m_CurrentTexture->Bind(0);
        m_ShaderPtr->Uniform1i(0, g_DiffuseTextureUniformName);

        Renderer::RenderVisible(glm::translate(g_IdentityMatrix, m_ModelPos), *m_CurrentVertexManager, *m_ShaderPtr, exp_norms);

        if (is_block_selected)
            m_ShaderPtr->Uniform1i(false, g_EntitySelectedUniformName);
    }
}
