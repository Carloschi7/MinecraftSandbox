#include "GlStructure.h"
#include "Vertices.h"
#include "Block.h"

namespace GlCore
{
    //World definitions(load all the global opengl-related data)
    void LoadResources()
    {
        State& state = *pstate;
        Window& wnd = *state.game_window;
        Camera& cam = *state.camera;

        cam.SetVectors(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        cam.SetPerspectiveValues(glm::radians(45.0f), f32(wnd.Width()) / f32(wnd.Height()), 0.1f, Defs::g_RenderDistance);
        cam.SetKeyboardFunction(Defs::KeyboardFunction);
        cam.SetMouseFunction(Defs::MouseFunction);
        
        //Loading cubemap data (needs to be a std::vector)
        std::vector<std::string> skybox_files =
        {
            "assets/textures/ShadedBackground.png",
            "assets/textures/ShadedBackground.png",
            "assets/textures/BotBackground.png",
            "assets/textures/TopBackground.png",
            "assets/textures/ShadedBackground.png",
            "assets/textures/ShadedBackground.png",
        };
        state.cubemap = std::make_shared<CubeMap>(skybox_files, Defs::g_RenderDistance / 2.0f);
        state.cubemap_shader = std::make_shared<Shader>("assets/shaders/cubemap.shader");
        state.cubemap_shader->UniformMat4f(cam.GetProjMatrix(), "proj");

        //Loading crossaim data
        //We set the crossaim model matrix here, for now this shader is used only 
        //for drawing this
        VertexData rd = CrossAim();
        state.crossaim_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(f32), rd.lyt);
        state.crossaim_shader = std::make_shared<Shader>("assets/shaders/basic_overlay.shader");
        state.crossaim_shader->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), "model");

        //Load water stuff
        rd = WaterLayer();
        state.water_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(f32), rd.lyt);
        state.water_shader = std::make_shared<Shader>("assets/shaders/water.shader");
        state.water_shader->UniformMat4f(cam.GetProjMatrix(), "proj");

        LayoutElement el{ 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.water_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxWaterLayersCount,
            state.water_shader->GetAttributeLocation("model_pos"), el);

        //Init framebuffer
        rd = CubeForDepth();
        state.depth_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(f32), rd.lyt);
        state.shadow_framebuffer = std::make_shared<FrameBuffer>(g_DepthMapWidth, g_DepthMapHeight, FrameBufferType::DEPTH_ATTACHMENT);
        state.depth_shader = std::make_shared<Shader>("assets/shaders/basic_shadow.shader");

        //Instanced attribute for block positions in the depth shader
        el = { 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.depth_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
        state.depth_shader->GetAttributeLocation("model_depth_pos"), el);

        //Init inventory stuff
        rd = Inventory();
        state.inventory_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(f32), rd.lyt);
        state.inventory_shader = std::make_shared<Shader>("assets/shaders/inventory.shader");


        glm::mat4 inventory_proj = glm::ortho(0.0f, (f32)wnd.Width(), (f32)wnd.Height(), 0.0f);
        state.inventory_shader->UniformMat4f(inventory_proj, "proj");

        //and also the inventory entry VM
        rd = InventoryEntryData();
        state.inventory_entry_vm = std::make_shared<VertexManager>(rd.vertices.data(), rd.vertices.size() * sizeof(f32), rd.lyt);

        //Load block and drop resources
        VertexData cd = Cube();
        state.block_vm = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(f32), cd.lyt);
        state.block_shader = std::make_shared<Shader>("assets/shaders/basic_cube.shader");
        state.drop_vm = std::make_shared<VertexManager>(cd.vertices.data(), cd.vertices.size() * sizeof(f32), cd.lyt);
        state.drop_shader = std::make_shared<Shader>("assets/shaders/basic_collectable.shader");

        //Load textures
        using TextureLoaderType = std::pair<std::string, Defs::TextureBinding>;
        Utils::AVector<TextureLoaderType> textures
        {
            {"texture_dirt",     Defs::TextureBinding::TextureDirt},
            {"texture_grass",    Defs::TextureBinding::TextureGrass},
            {"texture_sand",     Defs::TextureBinding::TextureSand},
            {"texture_trunk",    Defs::TextureBinding::TextureWood},
            {"texture_leaves",   Defs::TextureBinding::TextureLeaves}
        };

        //The binding matches the vector position
        InitGameTextures(state.game_textures);
        for (auto& elem : textures)
        {
            u32 tex_index = static_cast<u32>(elem.second);
            state.game_textures[tex_index].Bind(tex_index);
            state.block_shader->Uniform1i(tex_index, elem.first);
            state.drop_shader->Uniform1i(tex_index, elem.first);
        }

        //Create instance buffer for model matrices
        el = { 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.block_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            state.block_shader->GetAttributeLocation("model_pos"), el);

        el = { 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(u32), 0 };
        state.block_vm->PushInstancedAttribute(nullptr, sizeof(u32) * g_MaxRenderedObjCount,
            state.block_shader->GetAttributeLocation("tex_index"), el);
    }


    void UpdateCamera()
    {
        pstate->camera->ProcessInput(*pstate->game_window, 1.0f);
    }

    void RenderSkybox()
    {
        //SetViewMatrix with no translation
        glm::mat4 view = glm::mat4(glm::mat3(pstate->camera->GetViewMatrix()));
        Renderer::Render(pstate->cubemap_shader, pstate->cubemap->GetVertexManager(), pstate->cubemap, view);
    
    }

    void RenderCrossaim()
    {
        Renderer::Render(pstate->crossaim_shader, *pstate->crossaim_vm, nullptr, {});
    }

    void UpdateShadowFramebuffer()
    {
        auto& pos = pstate->camera->GetPosition();
        f32 x = pos.x - std::fmod(pos.x, 32.0f);
        f32 z = pos.z - std::fmod(pos.z, 32.0f);
        glm::vec3 light_eye = glm::vec3(x, 500.0f, z);

        static glm::mat4 proj = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 0.1f, 550.0f);
        glm::mat4 view = glm::lookAt(light_eye, glm::vec3(light_eye.x, 0.0f, light_eye.z), g_PosZ);
        g_DepthSpaceMatrix = proj * view;

        pstate->depth_shader->UniformMat4f(g_DepthSpaceMatrix, "lightSpace");
    }

    void UniformProjMatrix()
    {
        pstate->block_shader->UniformMat4f(pstate->camera->GetProjMatrix(), "proj");
        pstate->drop_shader->UniformMat4f(pstate->camera->GetProjMatrix(), "proj");
    }

    void UniformViewMatrix()
    {
        pstate->block_shader->UniformMat4f(pstate->camera->GetViewMatrix(), "view");
        pstate->drop_shader->UniformMat4f(pstate->camera->GetViewMatrix(), "view");
        pstate->water_shader->UniformMat4f(pstate->camera->GetViewMatrix(), "view");
    }

    void InitGameTextures(std::vector<Texture>& textures)
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
}
