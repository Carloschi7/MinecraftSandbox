#include "../application/Application.h"
#include "Entrypoint.h"
#include "World.h"
#include "Renderer.h"
#include "State.h"
#include "InventorySystem.h"

#ifdef __linux__
#   include <experimental/filesystem>
#else
#   include <filesystem>
#endif

//We create the window alongside with the whole OpenGL context
Application::Application() :
    m_Window(InitContext(1920, 1080, "MinecraftClone", true))
{
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
    //Init global vars
    GlCore::State& state = GlCore::State::GetState();
    state.SetGameWindow(&m_Window);
    state.SetGameCamera(&m_Camera);
    static bool state_switch = false;

    //Game data
    World world_instance;
    Inventory game_inventory;

    auto switch_game_state = [&]() 
    {
        if (Defs::g_ViewMode == Defs::ViewMode::WorldInteraction)
            Defs::g_ViewMode = Defs::ViewMode::Inventory;
        else
            Defs::g_ViewMode = Defs::ViewMode::WorldInteraction;

        state_switch = true;
    };

    //Logic thread function
    auto logic_thread_impl = [&]()
    {
        Utils::Timer timer;
        float recorded_time = 0.1f;
        while (GlCore::g_LogicThreadShouldRun)
        {
            timer.StartTimer();
            //The gamemode check is here because the key state update is reqwuired by the logic thread
            if (m_Window.IsKeyPressed(GLFW_KEY_SPACE))
                switch_game_state();

            world_instance.UpdateScene(game_inventory, recorded_time);
            game_inventory.HandleInventorySelection();
            state.GameWindow().UpdateKeys();
            //Update local timer
            recorded_time = timer.GetElapsedSeconds();
        }
    };

    if constexpr (GlCore::g_MultithreadedRendering)
    {
        m_AppThreads.emplace_back(logic_thread_impl);
        //Populate thread pool with static threads (currently unused)
        GlCore::g_ThreadPool.insert({ "Renderer thread", std::this_thread::get_id() });
        GlCore::g_ThreadPool.insert({ "Logic thread", m_AppThreads[0].get_id() });
    }

    //Set blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_MIN);
    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    Utils::Timer timer;
    //Standard value for the first frame
    float recorded_time = 0.1f;
    while (!m_Window.ShouldClose())
    {
        if constexpr (!GlCore::g_MultithreadedRendering)
            if (m_Window.IsKeyPressed(GLFW_KEY_SPACE))
                switch_game_state();

        timer.StartTimer();
        if constexpr (GlCore::g_MultithreadedRendering)
        {
            //No need to render every frame, while the logic thread computes,
            //sleeping every few milliseconds can save lots of performances without
            //resulting too slow
            world_instance.Render();
        }
        else
        {
            world_instance.UpdateScene(game_inventory, recorded_time);
            game_inventory.HandleInventorySelection();
            state.GameWindow().UpdateKeys();
            world_instance.Render();
        }
        game_inventory.ScreenSideRender();

        if (Defs::g_ViewMode == Defs::ViewMode::Inventory)
        {
            if (state_switch)
            {
                state.GameWindow().EnableCursor();
                state_switch = false;
            }
            
            game_inventory.InternalSideRender();
        }
        else
        {
            if (state_switch)
            {
                state.GameWindow().DisableCursor();
                state_switch = false;
            }

            recorded_time = timer.GetElapsedSeconds();
            m_Camera.ProcessInput(m_Window, recorded_time * Defs::g_FramedPlayerSpeed, 0.8);
        }

        //CAMERA DEBUG
        //std::cout << m_Camera.GetPosition().x << ", " << m_Camera.GetPosition().z << std::endl;
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

    path p{ "runtime_files" };
    remove_all(p);
}