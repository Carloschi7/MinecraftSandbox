#include "GlStructure.h"
#include "Vertices.h"
#include "State.h"
#include "algs_3d.h"
#include "Renderer.h"

namespace GlCore
{
    //World definitions(load all the global opengl-related data)
    void LoadResources()
    {
        State& state = *pstate;
        Window& wnd = *state.game_window;
        Camera& cam = *state.camera;
        MeshStorage& stg = *state.mesh_storage;
        Memory::Arena* allocator = state.memory_arena;


        glm::vec3 spawn_coordinates(0.0f, 115.0f, 0.0f);

        cam.SetVectors(spawn_coordinates, glm::vec3(0.0f, 0.0f, -1.0f));
        cam.SetPerspectiveValues(glm::radians(g_FovDegrees), f32(wnd.Width()) / f32(wnd.Height()), 0.1f, Defs::g_RenderDistance);
        cam.SetKeyboardFunction(Defs::KeyboardFunction);
        cam.SetMouseFunction(Defs::MouseFunction);
        
        //Loading cubemap data (needs to be a std::vector)
        std::vector<std::string> skybox_files =
        {
            Utils::CompletePath("assets/textures/ShadedBackground.png"),
            Utils::CompletePath("assets/textures/ShadedBackground.png"),
            Utils::CompletePath("assets/textures/BotBackground.png"),
            Utils::CompletePath("assets/textures/TopBackground.png"),
            Utils::CompletePath("assets/textures/ShadedBackground.png"),
            Utils::CompletePath("assets/textures/ShadedBackground.png"),
        };
        state.cubemap = Memory::NewUnchecked<CubeMap>(allocator, skybox_files, Defs::g_RenderDistance / 2.0f);
        state.cubemap_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/cubemap.shader"));
        state.cubemap_shader->UniformMat4f(cam.GetProjMatrix(), "proj");

        //Loading crossaim data
        //We set the crossaim model matrix here, for now this shader is used only 
        //for drawing this
        MeshElement* elem = &stg.crossaim;
        state.crossaim_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.crossaim_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/basic_overlay.shader"));
        state.crossaim_shader->UniformMat4f(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)), "model");

        //Load water stuff
        elem = &stg.water_layer;
        state.water_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.water_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/water.shader"));
        state.water_shader->UniformMat4f(cam.GetProjMatrix(), "proj");

        LayoutElement instanced_layout_element{ 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.water_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxWaterLayersCount,
            state.water_shader->GetAttributeLocation("model_pos"), instanced_layout_element);

        //Init framebuffer
        elem = &stg.pos_and_tex_coord_depth;
        state.depth_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.shadow_framebuffer = Memory::NewUnchecked<FrameBuffer>(allocator, g_DepthMapWidth, g_DepthMapHeight, FrameBufferType::DEPTH_ATTACHMENT);
        state.depth_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/basic_shadow.shader"));

        //Instanced attribute for block positions in the depth shader
        instanced_layout_element = { 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.depth_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
        state.depth_shader->GetAttributeLocation("model_depth_pos"), instanced_layout_element);

        //Init inventory stuff
        elem = &stg.inventory;
        state.inventory_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.inventory_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/inventory.shader"));


        glm::mat4 inventory_proj = glm::ortho(0.0f, (f32)wnd.Width(), (f32)wnd.Height(), 0.0f);
        state.inventory_shader->UniformMat4f(inventory_proj, "proj");
        state.inventory_shader->Uniform1i(-1, "optional_texture_index");

        //and also the inventory entry VM
        elem = &stg.inventory_entry;
        state.inventory_entry_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);

        elem = &stg.pos_and_tex_coords_decal2d;
        state.decal2d_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);

        //Screen texture
        elem = &stg.pos_and_tex_coords_screen;
        state.screen_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.screen_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/screen.shader"));
        //state.screen_shader->UniformMat4f(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 15.0f), "proj");
        state.screen_framebuffer = Memory::NewUnchecked<FrameBuffer>(allocator, Defs::g_ScreenWidth, Defs::g_ScreenHeight, FrameBufferType::COLOR_ATTACHMENT);

        //Load block and drop resources
        elem = &stg.pos_and_tex_coord_default;
        state.block_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.block_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/scene.shader"));
        state.drop_vm = Memory::NewUnchecked<VertexManager>(allocator, elem->data, elem->count * sizeof(f32), elem->lyt);
        state.drop_shader = Memory::NewUnchecked<Shader>(allocator, Utils::CompletePath("assets/shaders/basic_collectable.shader"));

        //Load textures
        InitGameTextures(state.game_textures);
        const u32 global_texture_index = static_cast<u32>(Defs::TextureBinding::GlobalTexture);
        state.game_textures[global_texture_index].Bind(global_texture_index);
        state.block_shader->Uniform1i(global_texture_index, "global_texture");
        state.drop_shader->Uniform1i(global_texture_index, "global_texture");

        //Create instance buffer for positions and texindices
        instanced_layout_element = { 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0 };
        state.block_vm->PushInstancedAttribute(nullptr, sizeof(glm::vec3) * g_MaxRenderedObjCount,
            state.block_shader->GetAttributeLocation("model_pos"), instanced_layout_element);

        instanced_layout_element = { 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(u32), 0 };
        state.block_vm->PushInstancedAttribute(nullptr, sizeof(u32) * g_MaxRenderedObjCount,
            state.block_shader->GetAttributeLocation("tex_index"), instanced_layout_element);

        TextureOffsets global_texture_offsets = LoadGlobalTextureOffsets();
        auto& offsets = global_texture_offsets.offsets;

        for (u32 i = 0; i < global_texture_offsets_count; i++) {
            state.block_shader->UniformVec2f(offsets[i], "item_offsets[" + std::to_string(i) + "]");
            state.drop_shader->UniformVec2f(offsets[i], "item_offsets[" + std::to_string(i) + "]");
            state.inventory_shader->UniformVec2f(offsets[i], "item_offsets[" + std::to_string(i) + "]");
        }
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

    void RenderHeldItem(Defs::Item sprite)
    {
        State& state = *pstate;
        Camera& camera = *pstate->camera;
        bool is_block = Defs::IsBlock(sprite);

        state.drop_shader->Use();
        state.drop_shader->Uniform1i(static_cast<u32>(sprite), "drop_texture_index");
        const VertexManager* cur = is_block ? state.drop_vm : state.decal2d_vm;
        cur->BindVertexArray();

        //Rotate the block selected to the bottom-right section of the player view
        f32 theta = Utils::PerspectiveItemRotation(g_FovDegrees, is_block);
        f32 extra_rotation = is_block ? 0.0f : glm::radians(-75.0f);

        glm::vec3 relative_up = camera.ComputeRelativeUp();
        const glm::vec3& front = camera.GetFront();
        glm::vec3 new_rot_axis = Algs3D::RotateVector(relative_up, front, -theta);
        glm::vec3 translation = Algs3D::RotateVector(front, new_rot_axis, theta) - glm::vec3(0.0f, 0.1f, 0.0f);

        glm::mat4 position_mat = glm::translate(glm::mat4(1.0f), camera.position + translation);
        //rotation to the view perspective
        position_mat = glm::rotate(position_mat, camera.RotationY(), glm::cross(camera.GetFront(), relative_up));
        position_mat = glm::rotate(position_mat, -camera.RotationX() + extra_rotation, glm::vec3(0.0f, 1.0f, 0.0f));

        position_mat = glm::scale(position_mat, glm::vec3(0.5f));
        state.drop_shader->UniformMat4f(position_mat, "model");

        glDrawArrays(GL_TRIANGLES, 0, cur->GetIndicesCount());
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
        glm::vec3 light_eye = glm::vec3(x, 600.0f, z);

        static glm::mat4 proj = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, 0.1f, 650.0f);
        glm::mat4 view = glm::lookAt(light_eye, glm::vec3(light_eye.x, 0.0f, light_eye.z), glm::vec3(0.0f, 0.0f, 1.0f));
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
        using Utils::CompletePath;
        textures.emplace_back(CompletePath("assets/textures/blocks_and_items.png").c_str(), false, TextureFilter::Nearest);
        textures.emplace_back(CompletePath("assets/textures/inventory.png").c_str(), false, TextureFilter::Nearest);
        textures.emplace_back(CompletePath("assets/textures/crafting_table_inventory.png").c_str(), false, TextureFilter::Nearest);
        textures.emplace_back(CompletePath("assets/textures/ScreenInventory.png").c_str(), true, TextureFilter::Nearest);
        textures.emplace_back(CompletePath("assets/textures/ScreenInventorySelector.png").c_str(), true, TextureFilter::Nearest);
    }
}
