#include "BsfPch.h"

#include "Application.h"
#include "Log.h"
#include "Scene.h"
#include "Assets.h"


namespace bsf
{
    

#pragma region GLFW Callbacks


#define BD_APP(x) (static_cast<::bsf::Application*>(glfwGetWindowUserPointer(x)))


    static void GLFW_Key(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            BD_APP(window)->KeyPressed.Emit({ key, action == GLFW_REPEAT });
        }
        else
        {
            BD_APP(window)->KeyReleased.Emit({ key });
        }
    }
    static void GLFW_CursorPos(GLFWwindow* window, double x, double y)
    {
        BD_APP(window)->MouseMoved.Emit({ float(x), float(y), MouseButton::None });
    }

    static void GLFW_MouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        MouseButton appButton = MouseButton::None;

        switch (button)
        {
        case GLFW_MOUSE_BUTTON_1: appButton = MouseButton::Left; break;
        case GLFW_MOUSE_BUTTON_2: appButton = MouseButton::Right; break;
        }

        if (action == GLFW_PRESS)
        {
            BD_APP(window)->MousePressed.Emit({ float(x), float(y), appButton });
        }
        else if (action == GLFW_RELEASE)
        {
            BD_APP(window)->MouseReleased.Emit({ float(x), float(y), appButton });
        }

    }

    static void GLFW_WindowSize(GLFWwindow* window, int w, int h)
    {
        BD_APP(window)->WindowResized.Emit({ float(w), float(h) });
    }

#undef BD_APP

#pragma endregion



    Application::Application() :
        m_Window(nullptr),
        m_CurrentScene(nullptr),
        m_NextScene(nullptr)
    {
    }

    Application::~Application()
    {
        m_CurrentScene = nullptr;
        m_NextScene = nullptr;
        Assets::Get().Dispose();
        glfwTerminate();
    }

    glm::vec2 Application::GetWindowSize() const
    {
        int w, h;
        glfwGetWindowSize(m_Window, &w, &h);

        return { w, h };
    }

    void Application::Start()
    {

        if (!glfwInit()) {
            BSF_ERROR("Can't initialize GLFW");
            return;
        }

        m_Window = glfwCreateWindow(1280, 780, "Hello World", NULL, NULL);
        if (!m_Window)
        {
            BSF_ERROR("Can't create the window");
            glfwTerminate();
        }

        glfwMakeContextCurrent(m_Window);

        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


        // Setup GLFW callbacks
        glfwSetWindowUserPointer(m_Window, this);

        glfwSetCursorPosCallback(m_Window, &GLFW_CursorPos);
        glfwSetMouseButtonCallback(m_Window, &GLFW_MouseButton);
        glfwSetWindowSizeCallback(m_Window, &GLFW_WindowSize);
        glfwSetKeyCallback(m_Window, &GLFW_Key);

        auto startTime = std::chrono::high_resolution_clock::now();
        auto prevTime = std::chrono::high_resolution_clock::now();
        auto currTime = std::chrono::high_resolution_clock::now();


        m_CurrentScene = std::make_shared<Scene>();
        m_CurrentScene->m_App = this;

        // Load Assets 
        Assets::Get().Load();

        while (!glfwWindowShouldClose(m_Window))
        {

            if (m_NextScene != nullptr)
            {
                m_CurrentScene->OnDetach();
                
                m_CurrentScene = std::move(m_NextScene); // m_NextScene = nullptr implicit
                m_CurrentScene->m_App = this;
                m_CurrentScene->OnAttach();

                // Reset time on scene change
                startTime = std::chrono::high_resolution_clock::now();
                prevTime = std::chrono::high_resolution_clock::now();
            }

            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> delta = now - prevTime;
            std::chrono::duration<float> elapsed = now - startTime;
            prevTime = now;

            m_CurrentScene->OnRender({ delta.count(), elapsed.count() });

            glfwSwapBuffers(m_Window);

            glfwPollEvents();
        }

        m_CurrentScene->OnDetach();

        m_CurrentScene = nullptr;

    }

    void Application::GotoScene(const std::shared_ptr<Scene> scene)
    {
        m_NextScene = scene;
    }

}