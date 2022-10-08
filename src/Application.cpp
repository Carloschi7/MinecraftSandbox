#include "../application/Application.h"
#include "World.h"

std::unique_ptr<Window> WindowMaker::MakeWindow()
{
    return std::make_unique<Window>(1920, 1080, "Application", true);
}

Application::Application(Window& window)
    :m_Window(window)
{
}

Application::~Application()
{
}

void Application::OnUserCreate()
{
    m_Window.SetVsync(true);
    m_Window.SetWndInCurrentContext();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void Application::OnUserRun()
{
    World WorldGameInstance(m_Window);

    m_Window.SetVsync(true);
    glEnable(GL_DEPTH_TEST);
    while (!m_Window.ShouldClose())
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        WorldGameInstance.UpdateScene();
        WorldGameInstance.DrawRenderable();
        m_Window.Update();
    }
}