#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"
#include "Renderer.h"

std::shared_ptr<Shader> GlCore::WorldStructure::m_CrossaimShaderPtr = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_CrossaimVmPtr = nullptr;

std::shared_ptr<VertexManager> GlCore::BlockStructure::m_VertexManagerPtr = nullptr;
std::shared_ptr<Shader> GlCore::BlockStructure::m_ShaderPtr = nullptr;
std::vector<Texture> GlCore::BlockStructure::m_Textures = {};

namespace GlCore
{
    //World definitions
    WorldStructure::WorldStructure(const Window& window)
        :m_GameWindow(window)
    {
        m_GameCamera.SetVectors(glm::vec3(-5.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f));

        m_GameCamera.SetPerspectiveValues(glm::radians(45.0f),
            float(m_GameWindow.Width()) / float(m_GameWindow.Height()),
            0.1f,
            100.0f);

        m_GameCamera.SetKeyboardFunction(GameDefs::KeyboardFunction);
        m_GameCamera.SetMouseFunction(GameDefs::MouseFunction);

        //Loading every crossaim asset needed
        if (m_CrossaimShaderPtr.get() == nullptr)
        {
            m_CrossaimShaderPtr = std::make_shared<Shader>("assets/shaders/basic_overlay.shader");
            //We set the crossaim model matrix here, for now this shader is used only 
            //for drawing this
            m_CrossaimShaderPtr->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), "model");

            Utils::VertexData rd = Utils::CrossAim();
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
        return rd;
    }

    GameDefs::ChunkBlockLogicData WorldStructure::GetChunkBlockLogicData() const
    {
        GameDefs::ChunkBlockLogicData ld;
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

    void WorldStructure::RenderCrossaim() const
    {
        Renderer::Render({}, m_CrossaimVmPtr, m_CrossaimShaderPtr, {});
    }

    ChunkStructure::ChunkStructure()
    {
    }

    void ChunkStructure::BlockRenderInit(const GameDefs::RenderData& rd, std::shared_ptr<Shader> block_shader) const
    {
        //Uniforming VP matrices
        block_shader->UniformMat4f(rd.proj_matrix, "proj");
        block_shader->UniformMat4f(rd.view_matrix, "view");
    }


    BlockStructure::BlockStructure()
    {
        //Just need to be loaded once, every block shares this properies
        if(m_VertexManagerPtr.get() == nullptr)
            InitVertexManager();
        
        if (m_ShaderPtr.get() == nullptr)
            InitShader();

        if(m_Textures.empty())
            InitTextures();
    }

    const std::vector<Texture>& BlockStructure::GetBlockTextures() const
    {
        return m_Textures;
    }

    std::shared_ptr<Shader> BlockStructure::GetShader() const
    {
        return m_ShaderPtr;
    }

    void BlockStructure::Draw(const glm::vec3& pos, const GameDefs::BlockType& bt, bool is_block_selected) const
    {
        Texture* current_texture = nullptr;

        if (is_block_selected)
            m_ShaderPtr->Uniform1i(true, "entity_selected");

        switch (bt)
        {
        case GameDefs::BlockType::DIRT:
            current_texture = &m_Textures[0];
            break;
        default:
            throw std::runtime_error("Texture preset for this block not found!");
        }

        //Actual block rendering
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        Renderer::Render(model, m_VertexManagerPtr, m_ShaderPtr, { { current_texture, "diffuse_texture" } });

        if (is_block_selected)
            m_ShaderPtr->Uniform1i(false, "entity_selected");
    }

    void BlockStructure::InitVertexManager()
    {
        Utils::VertexData cd = Utils::Cube(Utils::VertexProps::POS_AND_TEX_COORDS);
        m_VertexManagerPtr = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
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
