#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"

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

    Gd::RenderData WorldStructure::GetRenderFrameInfo()
    {
        Gd::RenderData rd;
        Camera& cam = Root::GameCamera();

        rd.camera_position = cam.GetPosition();
        rd.proj_matrix = cam.GetProjMatrix();
        rd.view_matrix = cam.GetViewMatrix();
        rd.p_key = Root::GameWindow().IsKeyboardEvent({ GLFW_KEY_P, GLFW_PRESS });
        return rd;
    }

    Gd::ChunkLogicData WorldStructure::GetChunkLogicData()
    {
        Gd::ChunkLogicData ld;
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
            false};

        Renderer::Render(pl);
    }

    void WorldStructure::RenderCrossaim() const
    {
        RendererPayload pl{ {}, m_CrossaimVmPtr.get(), m_CrossaimShaderPtr.get(), nullptr, nullptr, nullptr, false };
        Renderer::Render(pl);
    }

    void WorldStructure::UniformRenderInit(const Gd::RenderData& rd) const
    {
        auto block_shader = Root::BlockShader();
        block_shader->UniformMat4f(rd.proj_matrix, g_ProjUniformName);
        block_shader->UniformMat4f(rd.view_matrix, g_ViewUniformName);
    }

    ChunkStructure::ChunkStructure()
    {
    }


    BlockStructure::BlockStructure(const glm::vec3& pos, const Gd::BlockType& bt)
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

        
    }

    const std::vector<Texture>& BlockStructure::GetBlockTextures() const
    {
        return m_Textures;
    }

    void BlockStructure::Draw(const glm::vec3& pos, const Gd::BlockType& bt,
        const DrawableData& exp_norms, bool is_block_selected) const
    {
        Texture* current_texture = nullptr;
        VertexManager* current_vertex_manager = nullptr;
        switch (bt)
        {
        case Gd::BlockType::DIRT:
            current_texture = &m_Textures[0];
            current_vertex_manager = m_VertexManagerSinglePtr.get();
            break;
        case Gd::BlockType::GRASS:
            current_texture = &m_Textures[1];
            current_vertex_manager = m_VertexManagerSidedPtr.get();
            break;
        default:
            throw std::runtime_error("Texture preset for this block not found!");
        }

        RendererPayload pl{ glm::translate(g_IdentityMatrix, pos),
                            current_vertex_manager,
                            m_ShaderPtr.get(),
                            current_texture,
                            nullptr,
                            &exp_norms,
                            is_block_selected};

        Renderer::Render(pl);
    }
}
