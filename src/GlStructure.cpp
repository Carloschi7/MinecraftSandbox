#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"

std::shared_ptr<CubeMap> GlCore::WorldStructure::m_CubemapPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CubemapShaderPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_CrossaimShaderPtr = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_CrossaimVmPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_WaterShader = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_WaterVmPtr = nullptr;
std::shared_ptr<FrameBuffer> GlCore::WorldStructure::m_DepthFramebufferPtr = nullptr;
std::shared_ptr<Shader> GlCore::WorldStructure::m_FramebufferShaderPtr = nullptr;
std::shared_ptr<VertexManager> GlCore::WorldStructure::m_DepthVMPtr = nullptr;

std::shared_ptr<VertexManager> GlCore::BlockStructure::m_VertexManagerPtr = nullptr;
std::shared_ptr<Shader> GlCore::BlockStructure::m_ShaderPtr = nullptr;

namespace GlCore
{
    //World definitions
    WorldStructure::WorldStructure() :
        m_State(State::GetState())
    {
        Camera& cam = m_State.GameCamera();
        cam.SetVectors(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

        cam.SetPerspectiveValues(glm::radians(45.0f),
            float(m_State.GameWindow().Width()) / float(m_State.GameWindow().Height()),
            0.1f,
            Gd::g_RenderDistance);

        cam.SetKeyboardFunction(Gd::KeyboardFunction);
        cam.SetMouseFunction(Gd::MouseFunction);
        
        //Static data already initialized
        if (m_CrossaimShaderPtr.get() != nullptr)
            return;
        
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
        m_CubemapShaderPtr->UniformMat4f(cam.GetProjMatrix(), "proj");

        //Loading crossaim data
        m_CrossaimShaderPtr = std::make_shared<Shader>("assets/shaders/basic_overlay.shader");
        //We set the crossaim model matrix here, for now this shader is used only 
        //for drawing this
        m_CrossaimShaderPtr->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), "model");

        VertexData rd = CrossAim();
        m_CrossaimVmPtr = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);

        //Load water stuff
        rd = WaterLayer();
        m_WaterVmPtr = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        m_WaterShader = std::make_shared<Shader>("assets/shaders/water.shader");
        m_WaterShader->UniformMat4f(cam.GetProjMatrix(), "proj");

        LayoutElement el{ 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        m_WaterVmPtr->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxWaterLayersCount,
            m_WaterShader->GetAttributeLocation("model_pos"), el);

        //Init framebuffer
        rd = CubeForDepth();
        m_DepthVMPtr = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);

        m_DepthFramebufferPtr = std::make_shared<FrameBuffer>(g_DepthMapWidth, g_DepthMapHeight, FrameBufferType::DEPTH_ATTACHMENT);
        m_FramebufferShaderPtr = std::make_shared<Shader>("assets/shaders/basic_shadow.shader");

        //Instanced attribute for block positions in the depth shader
        el = { 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        m_DepthVMPtr->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            m_FramebufferShaderPtr->GetAttributeLocation("model_depth_pos"), el);
        //Map the instance


        m_State.SetShadowFramebuffer(m_DepthFramebufferPtr);
        m_State.SetDepthShader(m_FramebufferShaderPtr);
        m_State.SetDepthVM(m_DepthVMPtr);
        m_State.SetWaterShader(m_WaterShader);
        m_State.SetWaterVM(m_WaterVmPtr);
    }


    void WorldStructure::UpdateCamera()
    {
        m_State.GameCamera().ProcessInput(m_State.GameWindow(), 1.0f);
    }

    void WorldStructure::RenderSkybox() const
    {
        //SetViewMatrix with no translation
        glm::mat4 view = glm::mat4(glm::mat3(m_State.GameCamera().GetViewMatrix()));
        Renderer::Render(m_CubemapShaderPtr, m_CubemapPtr->GetVertexManager(), m_CubemapPtr, view);
    }

    void WorldStructure::RenderCrossaim() const
    {
        Renderer::Render(m_CrossaimShaderPtr, *m_CrossaimVmPtr, nullptr, {});
    }

    void WorldStructure::UpdateShadowFramebuffer() const
    {
        auto& pos = m_State.GameCamera().GetPosition();
        float x = pos.x - std::fmod(pos.x, 32.0f);
        float z = pos.z - std::fmod(pos.z, 32.0f);
        glm::vec3 light_eye = glm::vec3(x, 500.0f, z);

        static glm::mat4 proj = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 0.1f, 550.0f);
        glm::mat4 view = glm::lookAt(light_eye, glm::vec3(light_eye.x, 0.0f, light_eye.z), g_PosZ);
        g_DepthSpaceMatrix = proj * view;

        m_FramebufferShaderPtr->UniformMat4f(g_DepthSpaceMatrix, "lightSpace");
    }

    void WorldStructure::UniformProjMatrix() const
    {
        auto block_shader = m_State.BlockShader();
        block_shader->UniformMat4f(m_State.GameCamera().GetProjMatrix(), "proj");
    }

    void WorldStructure::UniformViewMatrix() const
    {
        auto block_shader = m_State.BlockShader();
        block_shader->UniformMat4f(m_State.GameCamera().GetViewMatrix(), "view");
        m_WaterShader->UniformMat4f(m_State.GameCamera().GetViewMatrix(), "view");
    }


    BlockStructure::BlockStructure(const glm::vec3& pos, const Gd::BlockType& bt)
    {
        //Load static data
        if (m_VertexManagerPtr.get() != nullptr)
            return;
        
        VertexData cd = Cube();
        m_VertexManagerPtr = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
        m_ShaderPtr = std::make_shared<Shader>("assets/shaders/basic_cube.shader");

        //Load textures
        using TextureLoaderType = std::pair<std::string, Gd::TextureBinding>;
        std::vector<TextureLoaderType> textures
        { 
            {"texture_dirt",     Gd::TextureBinding::TextureDirt},
            {"texture_grass",    Gd::TextureBinding::TextureGrass},
            {"texture_sand",     Gd::TextureBinding::TextureSand},
            {"texture_trunk",    Gd::TextureBinding::TextureWood},
            {"texture_leaves",   Gd::TextureBinding::TextureLeaves}
        };

        //The binding matches the vector position
        auto& game_textures = LoadGameTextures();
        for (auto& elem : textures)
        {
            uint32_t tex_index = static_cast<uint32_t>(elem.second);
            game_textures[tex_index].Bind(tex_index);
            m_ShaderPtr->Uniform1i(tex_index, elem.first);
        }

        //Create instance buffer for model matrices
        LayoutElement el{ 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        m_VertexManagerPtr->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            m_ShaderPtr->GetAttributeLocation("model_pos"), el);

        LayoutElement el2{ 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(uint32_t), 0 };
        m_VertexManagerPtr->PushInstancedAttribute(nullptr, sizeof(uint32_t) * g_MaxRenderedObjCount,
            m_ShaderPtr->GetAttributeLocation("tex_index"), el2);

        //Root management
        State& state = State::GetState();
        state.SetBlockShader(m_ShaderPtr);
        state.SetBlockVM(m_VertexManagerPtr);
    }
    const std::vector<Texture>& LoadGameTextures()
    {
        static std::vector<Texture> textures;

        if (textures.empty())
        {
            textures.emplace_back("assets/textures/dirt.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/grass.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/sand.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/trunk.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/leaves.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/water.png", false, TextureFilter::Nearest);
        }

        return textures;
    }
}
