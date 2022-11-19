#include "../application/Application.h"
#include "Entrypoint.h"
#include "World.h"
#include "Renderer.h"

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
    //Setting the window in the environment vars
    GlCore::Root::SetGameWindow(&m_Window);
    GlCore::Root::SetGameCamera(&m_Camera);
    GlCore::Renderer::Init();
}

void Application::OnUserRun()
{
    //TODO find a way to initialize this in the thread with the opnegl context
    World WorldGameInstance;

    //Logic thread function
    auto logic_thread_impl = [&]()
    {
        while (GlCore::g_LogicThreadShouldRun)
        {
            WorldGameInstance.UpdateScene();
        }
    };

    if constexpr (GlCore::g_MultithreadedRendering)
        m_AppThreads.emplace_back(logic_thread_impl);

    //For 3D rendering
    glEnable(GL_DEPTH_TEST);
    while (!m_Window.ShouldClose())
    {   
        m_Window.ClearScreen();
        if constexpr (GlCore::g_MultithreadedRendering)
        {
            //No need to render every frame, while the logic thread computes,
            //sleeping every few milliseconds can save lots of performances without
            //resulting too slow
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(5ms);
            WorldGameInstance.DrawRenderable();
        }
        else
        {
            WorldGameInstance.UpdateScene();
            WorldGameInstance.DrawRenderable();
        }

        m_Window.Update();
    }

    if constexpr (GlCore::g_MultithreadedRendering)
    {
        //Join the logic thread
        GlCore::g_LogicThreadShouldRun = false;
        m_AppThreads[0].join();
    }
}