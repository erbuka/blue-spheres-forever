#include "BsfPch.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Application.h"
#include "Log.h"
#include "Assets.h"
#include "Renderer2D.h"
#include "Audio.h"


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
        m_NextScene(nullptr),
        m_Renderer2D(nullptr)
    {
    }

    Application::~Application()
    {
        m_CurrentScene = nullptr;
        m_NextScene = nullptr;

        m_Renderer2D = nullptr;

        m_AudioMixer = nullptr;
        
        Assets::GetInstance().Dispose();
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

        // Initialize ImGui
        InitImGui();

        // Setup GLFW callbacks
        glfwSetWindowUserPointer(m_Window, this);

        glfwSetCursorPosCallback(m_Window, &GLFW_CursorPos);
        glfwSetMouseButtonCallback(m_Window, &GLFW_MouseButton);
        glfwSetWindowSizeCallback(m_Window, &GLFW_WindowSize);
        glfwSetKeyCallback(m_Window, &GLFW_Key);

        auto startTime = std::chrono::high_resolution_clock::now();
        auto prevTime = std::chrono::high_resolution_clock::now();
        auto currTime = std::chrono::high_resolution_clock::now();

        glfwSwapInterval(0);

        m_CurrentScene = std::make_shared<Scene>();
        m_CurrentScene->m_App = this;


        // Init Renderer 2D
        m_Renderer2D = MakeRef<Renderer2D>();

        // Init Audio Mixer
        m_AudioMixer = MakeRef<AudioDevice>();
        
        // Load Assets 
        Assets::GetInstance().Load();

        m_Running = true;

        while (!glfwWindowShouldClose(m_Window) && m_Running)
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
            const Time time = { delta.count(), elapsed.count() };


            /* FPS counter */
            char buffer[0x80];
            sprintf_s(buffer, "Period: %f, FPS: %f", delta.count(), 1.0f / delta.count());
            glfwSetWindowTitle(m_Window, buffer);
            /* FPS counter */

            if (m_CurrentScene->m_ImGui)
            {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
            }
            
            RunScheduledTasks(time, m_CurrentScene, ESceneTaskEvent::PreRender);
            m_CurrentScene->OnRender(time);
            RunScheduledTasks(time, m_CurrentScene, ESceneTaskEvent::PostRender);

            if (m_CurrentScene->m_ImGui)
            {
                auto windowSize = GetWindowSize();
                glViewport(0, 0, windowSize.x, windowSize.y);
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }


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
        return *(m_AudioMixer.get());
    }

	void bsf::Application::InitImGui()
	{
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        
	}


	void bsf::Application::RunScheduledTasks(const Time& time, const Ref<Scene>& scene, ESceneTaskEvent evt)
    {
        auto& tasks = scene->m_ScheduledTasks[evt];
        auto taskIt = tasks.begin();

        for (auto taskIt = tasks.begin(); taskIt != tasks.end();)
        {
            auto& task = (*taskIt);
            task->m_Application = this;
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