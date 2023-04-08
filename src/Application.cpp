#include "../application/Application.h"
#include "Entrypoint.h"
#include "World.h"
#include "Renderer.h"
#include "State.h"
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

    World WorldGameInstance;
    static bool state_switch = false;

    auto switch_game_state = [&]() 
    {
        if (Gd::g_GameMode == Gd::ViewMode::WorldInteraction)
            Gd::g_GameMode = Gd::ViewMode::Inventory;
        else
            Gd::g_GameMode = Gd::ViewMode::WorldInteraction;

        state_switch = true;
    };

    //Logic thread function
    auto logic_thread_impl = [&]()
    {
        Utils::Timer timer;
        while (GlCore::g_LogicThreadShouldRun)
        {
            //The gamemode check is here because the key state update is reqwuired by the logic thread
            if (m_Window.IsKeyPressed(GLFW_KEY_SPACE))
                switch_game_state();

            WorldGameInstance.UpdateScene();
            state.GameWindow().UpdateKeys();
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
            WorldGameInstance.Render();
        }
        else
        {
            WorldGameInstance.UpdateScene();
            WorldGameInstance.Render();
        }

        if (Gd::g_GameMode == Gd::ViewMode::Inventory)
        {
            if (state_switch)
            {
                state.GameWindow().EnableCursor();
                state_switch = false;
            }

            GlCore::RenderInventory();
            Gd::HandleInventorySelection();
        }
        else
        {
            if (state_switch)
            {
                state.GameWindow().DisableCursor();
                state_switch = false;
            }

            m_Camera.ProcessInput(m_Window, std::max(0.01f, timer.GetElapsedSeconds()) * Gd::g_FramedPlayerSpeed, 0.8);
        }

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