#include "../application/Application.h"
#include "Entrypoint.h"
#include "World.h"
#include "Renderer.h"
#include "State.h"
#include "InventorySystem.h"
#include "Memory.h"


#ifdef __linux__
#   include <experimental/filesystem>
#else
#   include <filesystem>
#endif

//We create the window alongside with the whole OpenGL context
Application::Application() :
    m_Window(InitContext(1920, 1080, "MinecraftClone", false))
{
    Defs::g_ScreenWidth = 1920;
    Defs::g_ScreenHeight = 1080;
    m_Window.AttachWndToCurrentContext();
}

Application::~Application()
{
    for (auto& thread : m_AppThreads)
        if(thread.joinable())
            thread.join();

    TerminateContext(m_Window);
}

void Application::OnUserCreate()
{
    GlCore::Renderer::Init();

    //Create needed dirs
#ifdef __linux__
    using namespace std::experimental::filesystem;
#else
    using namespace std::filesystem;
#endif

    path p{ "runtime_files" };
    if(!is_directory(p))
        create_directory(p);
}

void Application::OnUserRun()
{
    //Init memory mapped_space
    //We will use 800mb of ram for this program
    Memory::Arena* mem_arena = Memory::InitializeArena(800 * 1024 * 1024);

    {
        //Init global vars
        GlCore::State state;
        GlCore::pstate = &state;

        state.game_window = &m_Window;
        state.camera = &m_Camera;
        state.memory_arena = mem_arena;
        bool state_switch = false;

        //Initialize OpenGL meshes
        GlCore::MeshStorage mesh_storage = GlCore::AllocateMeshStorage(mem_arena);
        state.mesh_storage = &mesh_storage;

        //Game data
        World world_instance;
        TextRenderer info_text_renderer{ {m_Window.Width(), m_Window.Height() }, static_cast<u32>(Defs::TextureBinding::TextureText) };
        Inventory game_inventory{ info_text_renderer };

        auto switch_game_state = [&]()
        {
            if (Defs::g_ViewMode == Defs::ViewMode::WorldInteraction)
                Defs::g_ViewMode = Defs::ViewMode::Inventory;
            else {
                game_inventory.ViewCleanup();
                game_inventory.view_crafting_table = false;
                Defs::g_ViewMode = Defs::ViewMode::WorldInteraction;
            }

            state_switch = true;
        };

        f32 thread1_record = 0.0f;
        f32 thread2_record = 0.0f;
        Utils::Timer thread1_record_timer;
        Utils::Timer thread2_record_timer;

        //Logic thread function
        auto logic_thread_impl = [&]()
        {
            Utils::Timer timer;
            f32 elapsed_time = 0.1f;
            thread1_record_timer.StartTimer();
            while (GlCore::g_LogicThreadShouldRun)
            {
                timer.StartTimer();
                //The gamemode check is here because the key state update is reqwuired by the logic thread
                if (m_Window.IsKeyPressed(Defs::g_InventoryKey))
                    switch_game_state();

                WorldEvent world_event = world_instance.UpdateScene(game_inventory, elapsed_time);
                if (world_event.crafting_table_open_command) {
                    game_inventory.view_crafting_table = true;
                    switch_game_state();
                }

                game_inventory.HandleInventorySelection();
                state.game_window->UpdateKeys();
                //Update local elapsed_timer
                elapsed_time = timer.GetElapsedSeconds();
                if (thread2_record_timer.GetElapsedMilliseconds() > 500.0f) {
                    thread2_record = elapsed_time;
                    thread2_record_timer.StartTimer();
                }
            }
        };

        if constexpr (GlCore::g_MultithreadedRendering) {
            //this line starts the thread btw
            m_AppThreads.emplace_back(logic_thread_impl);
        }

        //Set blending function
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_MIN);
        glDisable(GL_BLEND);

        Utils::Timer elapsed_timer;
        thread1_record_timer.StartTimer();
        //Standard value for the first frame
        f32 elapsed_time = 0.1f;
        while (!m_Window.ShouldClose())
        {
            if constexpr (!GlCore::g_MultithreadedRendering)
                if (m_Window.IsKeyPressed(Defs::g_InventoryKey))
                    switch_game_state();

            elapsed_timer.StartTimer();
            //No need to render every frame, while the logic thread computes,
            //sleeping every few milliseconds can save lots of performances without
            //resulting too slow
            if constexpr (!GlCore::g_MultithreadedRendering)
            {
                //The gamemode check is here because the key state update is reqwuired by the logic thread
                if (m_Window.IsKeyPressed(Defs::g_InventoryKey))
                    switch_game_state();

                WorldEvent world_event = world_instance.UpdateScene(game_inventory, elapsed_time);
                if (world_event.crafting_table_open_command) {
                    game_inventory.view_crafting_table = true;
                    switch_game_state();
                }

                game_inventory.HandleInventorySelection();
                state.game_window->UpdateKeys();
            }

            //Rendering ---------------------
            GlCore::UniformViewMatrix();

            //Copy these vectors to make the camera indipendent from the logic thread
            glm::vec3 camera_position = m_Camera.position;
            glm::vec3 camera_direction = m_Camera.GetFront();
            world_instance.Render(game_inventory, camera_position, camera_direction);
            game_inventory.ScreenRender();

            if (Defs::g_ViewMode == Defs::ViewMode::Inventory) {
                if (state_switch) {
                    state.game_window->EnableCursor();
                    state_switch = false;
                }

                game_inventory.InternalRender();
                //Done to keep the same view angle after using the inventory
                state.camera->UpdateMousePosition(*state.game_window);
            }
            else {
                if (state_switch) {
                    state.game_window->DisableCursor();
                    state_switch = false;
                }

                Physics::ProcessPlayerAxisMovement(elapsed_time);
                Physics::HandlePlayerMovement(elapsed_time);
                if (Defs::g_MovementType != Defs::MovementType::Creative)
                    Physics::HandlePlayerGravity(elapsed_time);

                world_instance.CheckPlayerCollision(camera_position);
            }

            //Timing & logging
            elapsed_time = elapsed_timer.GetElapsedSeconds();
            if (thread1_record_timer.GetElapsedMilliseconds() > 500.0f) {
                thread1_record = elapsed_time;
                thread1_record_timer.StartTimer();
            }
#ifndef MC_STANDALONE
            info_text_renderer.DrawString("Thread-1:" + std::to_string(thread1_record * 1000.0f) + "ms", { 0, 0 });
            info_text_renderer.DrawString("Thread-2:" + std::to_string(thread2_record * 1000.0f) + "ms", { 0, 40 });
            info_text_renderer.DrawString("mem:" + std::to_string(static_cast<f32>(mem_arena->mapped_space.memory_used) / (1024.f * 1024.f)) + "MB", { 0,80 });
#endif
            m_Window.Update();
        }

        if constexpr (GlCore::g_MultithreadedRendering)
        {
            //Join the logic thread
            GlCore::g_LogicThreadShouldRun = false;
            m_AppThreads[0].join();
        }

        //Remove serialization dir
#ifdef __linux__
        using namespace std::experimental::filesystem;
#else
        using namespace std::filesystem;
#endif

        GlCore::FreeMeshStorage(mesh_storage, mem_arena);
        path p{ "runtime_files" };
        remove_all(p); 
    }
    //Be sure no one will use this anymore, shutdown phase
    GlCore::pstate = nullptr;

    //Check for memory leaks (done only in debug mode, just to check if there are leaks during application runtime that need to be fixed)
#ifdef _DEBUG
    MC_ASSERT(mem_arena->mapped_space.memory_used == mem_arena->unfreed_mem,
        "There are some memory leaks during application shutdown, please fix them, as they may leak some data outside the arena");
#endif
    Memory::DestroyArena(mem_arena);
}