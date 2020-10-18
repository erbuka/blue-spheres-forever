#include "BsfPch.h"

#include "Application.h"
#include "Log.h"
#include "Assets.h"
#include "Renderer2D.h"
#include "Audio.h"
#include "Diagnostic.h"


namespace bsf
{
    

#pragma region GLFW Callbacks


#define BD_APP(x) (static_cast<::bsf::Application*>(glfwGetWindowUserPointer(x)))

    static void GLFW_Scroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        BD_APP(window)->Wheel.Emit({ (float)xoffset, (float)yoffset, (float)x, (float)y });
    }

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
        BD_APP(window)->MouseMoved.Emit({ float(x), float(y), 0, 0, MouseButton::None });
    }

    static void GLFW_MouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        MouseButton appButton = MouseButton::None;

        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT: appButton = MouseButton::Left; break;
        case GLFW_MOUSE_BUTTON_RIGHT: appButton = MouseButton::Right; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: appButton = MouseButton::Middle; break;
        }

        if (action == GLFW_PRESS)
        {
            BD_APP(window)->MousePressed.Emit({ float(x), float(y), 0, 0, appButton });
        }
        else if (action == GLFW_RELEASE)
        {
            BD_APP(window)->MouseReleased.Emit({ float(x), float(y), 0, 0, appButton });
        }

    }

    static void GLFW_WindowSize(GLFWwindow* window, int w, int h)
    {
        BD_APP(window)->WindowResized.Emit({ float(w), float(h) });
    }

    static void GLFW_Char(GLFWwindow* window, uint32_t codePoint)
    {
        BD_APP(window)->CharacterTyped.Emit({ (char)codePoint });
    }

#undef BD_APP

#pragma endregion



    Application::Application() :
        m_Window(nullptr),
        m_CurrentScene(nullptr),
        m_NextScene(nullptr),
        m_Renderer2D(nullptr)
    {
    }

    Application::~Application()
    {
        m_CurrentScene = nullptr;
        m_NextScene = nullptr;

        m_Renderer2D = nullptr;

        m_AudioDevice = nullptr;
        
        Assets::GetInstance().Dispose();
        glfwTerminate();
    }

    glm::vec2 Application::GetWindowSize() const
    {
        int w, h;
        glfwGetWindowSize(m_Window, &w, &h);

        return { w, h };
    }

    glm::vec2 bsf::Application::GetMousePosition() const
    {
        double x, y;
        glfwGetCursorPos(m_Window, &x, &y);
        return { x, y };
    }

    bool bsf::Application::GetKeyPressed(int32_t key) const
    {
        return glfwGetKey(m_Window, key) == GLFW_PRESS;
    }

    void Application::Start()
    {
        if (!glfwInit()) 
        {
            BSF_ERROR("Can't initialize GLFW");
            return;
        }

        m_Window = glfwCreateWindow(1280, 780, "Blue Spheres Forever", NULL, NULL);
        
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
        glfwSetScrollCallback(m_Window, &GLFW_Scroll);
        glfwSetKeyCallback(m_Window, &GLFW_Key);
        glfwSetCharCallback(m_Window, &GLFW_Char);

        glfwSwapInterval(0);

        // Init diagnostic
        BSF_DIAGNOSTIC_INIT(this, m_Window);

        // Start with a default scene
        m_CurrentScene = std::make_shared<Scene>();
        m_CurrentScene->m_App = this;

        // Init Renderer 2D
        m_Renderer2D = MakeRef<Renderer2D>();

        // Init Audio Device
        m_AudioDevice = MakeRef<AudioDevice>();
        
        // Load Assets 
        Assets::GetInstance().Load();

        auto startTime = std::chrono::high_resolution_clock::now();
        auto prevTime = std::chrono::high_resolution_clock::now();
        auto currTime = std::chrono::high_resolution_clock::now();

        m_Running = true;

        while (!glfwWindowShouldClose(m_Window) && m_Running)
        {
            BSF_DIAGNOSTIC_BEGIN();

            {
                if (m_NextScene != nullptr)
                {
                    m_CurrentScene->ClearSubscriptions();
                    m_CurrentScene->OnDetach();
                    m_CurrentScene = std::move(m_NextScene); // m_NextScene = nullptr implicit
                    m_CurrentScene->m_App = this;
                    m_CurrentScene->OnAttach();

                    // Reset time on scene change
                    startTime = prevTime = std::chrono::high_resolution_clock::now();
                }

                auto now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> delta = now - prevTime;
                std::chrono::duration<float> elapsed = now - startTime;
                prevTime = now;
                const Time time = { delta.count(), elapsed.count() };


                /* FPS counter */
                char buffer[0x80];
                sprintf_s(buffer, "Period: %f, FPS: %f", delta.count(), 1.0f / delta.count());
                glfwSetWindowTitle(m_Window, buffer);
                /* FPS counter */

                RunScheduledTasks(time, m_CurrentScene, ESceneTaskEvent::PreRender);
                m_CurrentScene->OnRender(time);
                RunScheduledTasks(time, m_CurrentScene, ESceneTaskEvent::PostRender);
            }

            BSF_DIAGNOSTIC_END();


            glfwSwapBuffers(m_Window);

            glfwPollEvents();
        }

        m_CurrentScene->ClearSubscriptions();
        m_CurrentScene->OnDetach();

        m_CurrentScene = nullptr;

    }

    void bsf::Application::GotoScene(std::shared_ptr<Scene>&& scene)
    {
        m_NextScene = std::move(scene);
    }

    void Application::GotoScene(const std::shared_ptr<Scene>& scene)
    {
        m_NextScene = scene;
    }

    void bsf::Application::Exit()
    {
        m_Running = false;
    }

    Renderer2D& bsf::Application::GetRenderer2D()
    {
        return *(m_Renderer2D.get());
    }

    AudioDevice& bsf::Application::GetAudioDevice()
    {
        return *(m_AudioDevice.get());
    }


	void bsf::Application::RunScheduledTasks(const Time& time, const Ref<Scene>& scene, ESceneTaskEvent evt)
    {
        auto& tasks = scene->m_ScheduledTasks[evt];

        for (auto taskIt = tasks.begin(); taskIt != tasks.end();)
        {
            auto& task = (*taskIt);
            task->m_Application = this;
            task->m_Scene = scene.get();
            task->m_Event = evt;
            task->CallUpdateFn(time);
            if (task->IsDone()) 
            {
                task->CallDoneFn();
                taskIt = tasks.erase(taskIt);
            }
            else
                ++taskIt;
        }

    }

}