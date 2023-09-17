#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"

namespace GlCore
{
    //World definitions(load all the global opengl-related data)
    void LoadResources()
    {
        State& state = State::GetState();
        Window& wnd = state.GameWindow();

        Camera& cam = state.GameCamera();
        cam.SetVectors(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

        cam.SetPerspectiveValues(glm::radians(45.0f), float(wnd.Width()) / float(wnd.Height()), 0.1f, Defs::g_RenderDistance);
        cam.SetKeyboardFunction(Defs::KeyboardFunction);
        cam.SetMouseFunction(Defs::MouseFunction);
        
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
        auto cubemap = std::make_shared<CubeMap>(skybox_files, Defs::g_RenderDistance / 2.0f);
        auto cubemap_shd = std::make_shared<Shader>("assets/shaders/cubemap.shader");
        cubemap_shd->UniformMat4f(cam.GetProjMatrix(), "proj");

        //Loading crossaim data
        //We set the crossaim model matrix here, for now this shader is used only 
        //for drawing this
        VertexData rd = CrossAim();
        auto crossaim_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        auto crossaim_shd = std::make_shared<Shader>("assets/shaders/basic_overlay.shader");
        crossaim_shd->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), "model");

        //Load water stuff
        rd = WaterLayer();
        auto water_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        auto water_shd = std::make_shared<Shader>("assets/shaders/water.shader");
        water_shd->UniformMat4f(cam.GetProjMatrix(), "proj");

        LayoutElement el{ 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        water_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxWaterLayersCount,
            water_shd->GetAttributeLocation("model_pos"), el);

        //Init framebuffer
        rd = CubeForDepth();
        auto depth_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        auto depth_fbf = std::make_shared<FrameBuffer>(g_DepthMapWidth, g_DepthMapHeight, FrameBufferType::DEPTH_ATTACHMENT);
        auto depth_shd = std::make_shared<Shader>("assets/shaders/basic_shadow.shader");

        //Instanced attribute for block positions in the depth shader
        el = { 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        depth_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            depth_shd->GetAttributeLocation("model_depth_pos"), el);

        //Init inventory stuff
        rd = Inventory();
        auto inventory_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);
        auto inventory_shd = std::make_shared<Shader>("assets/shaders/inventory.shader");


        glm::mat4 inventory_proj = glm::ortho(0.0f, (float)wnd.Width(), (float)wnd.Height(), 0.0f);
        inventory_shd->UniformMat4f(inventory_proj, "proj");

        //and also the inventory entry VM
        rd = InventoryEntryData();
        auto inventory_entry_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(float), rd.lyt);

        //Load block and drop resources
        VertexData cd = Cube();
        auto block_vm = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
        auto block_shd = std::make_shared<Shader>("assets/shaders/basic_cube.shader");
        auto drop_vm = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(float), cd.lyt);
        auto drop_shd = std::make_shared<Shader>("assets/shaders/basic_collectable.shader");

        //Load textures
        using TextureLoaderType = std::pair<std::string, Defs::TextureBinding>;
        std::vector<TextureLoaderType> textures
        {
            {"texture_dirt",     Defs::TextureBinding::TextureDirt},
            {"texture_grass",    Defs::TextureBinding::TextureGrass},
            {"texture_sand",     Defs::TextureBinding::TextureSand},
            {"texture_trunk",    Defs::TextureBinding::TextureWood},
            {"texture_leaves",   Defs::TextureBinding::TextureLeaves}
        };

        //The binding matches the vector position
        auto& game_textures = GameTextures();
        for (auto& elem : textures)
        {
            u32 tex_index = static_cast<u32>(elem.second);
            game_textures[tex_index].Bind(tex_index);
            block_shd->Uniform1i(tex_index, elem.first);
            drop_shd->Uniform1i(tex_index, elem.first);
        }

        //Create instance buffer for model matrices
        el = { 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0 };
        block_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            block_shd->GetAttributeLocation("model_pos"), el);

        el = { 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(u32), 0 };
        block_vm->PushInstancedAttribute(nullptr, sizeof(u32) * g_MaxRenderedObjCount,
            block_shd->GetAttributeLocation("tex_index"), el);

        //Root management
        state.SetCubemap(cubemap);
        state.SetCubemapShader(cubemap_shd);
        state.SetCrossaimVM(crossaim_vm);
        state.SetCrossaimShader(crossaim_shd);
        state.SetShadowFramebuffer(depth_fbf);
        state.SetDepthShader(depth_shd);
        state.SetDepthVM(depth_vm);
        state.SetWaterShader(water_shd);
        state.SetWaterVM(water_vm);
        state.SetInventoryShader(inventory_shd);
        state.SetInventoryVM(inventory_vm);
        state.SetInventoryEntryVM(inventory_entry_vm);
        state.SetBlockShader(block_shd);
        state.SetBlockVM(block_vm);
        state.SetDropShader(drop_shd);
        state.SetDropVM(drop_vm);
    }


    void UpdateCamera()
    {
        State& state = State::GetState();
        state.GameCamera().ProcessInput(state.GameWindow(), 1.0f);
    }

    void RenderSkybox()
    {
        State& state = State::GetState();
        //SetViewMatrix with no translation
        glm::mat4 view = glm::mat4(glm::mat3(state.GameCamera().GetViewMatrix()));
        Renderer::Render(state.CubemapShader(), state.Cubemap()->GetVertexManager(), state.Cubemap(), view);
    
    }

    void RenderCrossaim()
    {
        State& state = State::GetState();
        Renderer::Render(state.CrossaimShader(), *state.CrossaimVM(), nullptr, {});
    }

    

    void UpdateShadowFramebuffer()
    {
        State& state = State::GetState();
        auto& pos = state.GameCamera().GetPosition();
        float x = pos.x - std::fmod(pos.x, 32.0f);
        float z = pos.z - std::fmod(pos.z, 32.0f);
        glm::vec3 light_eye = glm::vec3(x, 500.0f, z);

        static glm::mat4 proj = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 0.1f, 550.0f);
        glm::mat4 view = glm::lookAt(light_eye, glm::vec3(light_eye.x, 0.0f, light_eye.z), g_PosZ);
        g_DepthSpaceMatrix = proj * view;

        state.DepthShader()->UniformMat4f(g_DepthSpaceMatrix, "lightSpace");
    }

    void UniformProjMatrix()
    {
        State& state = State::GetState();
        state.BlockShader()->UniformMat4f(state.GameCamera().GetProjMatrix(), "proj");
        state.DropShader()->UniformMat4f(state.GameCamera().GetProjMatrix(), "proj");
    }

    void UniformViewMatrix()
    {
        State& state = State::GetState();
        state.BlockShader()->UniformMat4f(state.GameCamera().GetViewMatrix(), "view");
        state.DropShader()->UniformMat4f(state.GameCamera().GetViewMatrix(), "view");
        state.WaterShader()->UniformMat4f(state.GameCamera().GetViewMatrix(), "view");
    }

    const std::vector<Texture>& GameTextures()
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
            textures.emplace_back("assets/textures/Inventory.png", false, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/ScreenInventory.png", true, TextureFilter::Nearest);
            textures.emplace_back("assets/textures/ScreenInventorySelector.png", true, TextureFilter::Nearest);
        }

        return textures;
    }
}
