#include "BsfPch.h"

#ifdef BSF_ENABLE_DIAGNOSTIC

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Diagnostic.h"
#include "Log.h"
#include "GameScene.h"
#include "Application.h"
#include "Assets.h"
#include "Stage.h"
#include "Config.h"

namespace bsf
{
	struct DiagnosticTool::Impl
	{
	public:

		std::unordered_map<const char*, DiagnosticToolStats> Stats;

		void Initialize(Application* app, GLFWwindow* window)
		{
			m_Window = window;
			m_App = app;

			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;

			ImGui::StyleColorsDark();

			ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
			ImGui_ImplOpenGL3_Init("#version 130");
		}

		void Begin()
		{
			for (auto& item : Stats)
			{
				item.second.ExecutionTime = 0.0f;
				item.second.Calls = 0;
			}

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void End()
		{
			for (auto& item : Stats)
			{
				item.second.LastExecutionTimes.pop_back();
				item.second.LastExecutionTimes.push_front(0.0f);
			}

			for (auto& item : Stats)
			{
				auto& execTimes = item.second.LastExecutionTimes;
				execTimes.front() = item.second.ExecutionTime;
				item.second.MaxExecutionTime = std::max(item.second.MaxExecutionTime, item.second.ExecutionTime);
				item.second.MeanExecutionTime = std::accumulate(execTimes.begin(), execTimes.end(), 0.0f) / (float)(DiagnosticToolStats::Samples);
			}




			ImGui::Begin("Diagnostic Tool");


			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("Info"))
				{
					ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
					ImGui::Text("GL Version: %s", glGetString(GL_VERSION));
					ImGui::Text("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Timing"))
				{
					if (ImGui::Button("Reset"))
						Reset();

					ImGui::Columns(4);
					ImGui::SetColumnWidth(0, 400.0f);
					ImGui::SetColumnWidth(1, 100.0f);
					ImGui::SetColumnWidth(2, 100.0f);
					ImGui::SetColumnWidth(3, 100.0f);

					ImGui::Text("Function/Scope");
					ImGui::NextColumn();
					ImGui::Text("Avg Time");
					ImGui::NextColumn();
					ImGui::Text("Max Time");
					ImGui::NextColumn();
					ImGui::Text("Calls");
					ImGui::NextColumn();

					ImGui::Separator();

					for (const auto& [name, stats] : DiagnosticTool::Get().m_Impl->Stats)
					{
						;
						ImGui::Text(name);
						ImGui::NextColumn();
						ImGui::Text("%.3f ms", stats.MeanExecutionTime);
						ImGui::NextColumn();
						ImGui::Text("%.3f ms", stats.MaxExecutionTime);
						ImGui::NextColumn();
						ImGui::Text("%d", stats.Calls);
						ImGui::NextColumn();
					}

					ImGui::EndTabItem();

				}

				if (ImGui::BeginTabItem("Utility"))
				{
					ImGui::InputText("Stage number", m_StageNumber, sizeof(m_StageNumber));
					ImGui::SameLine();
					if (ImGui::Button("Play"))
					{
						uint32_t stageNumber = std::stoi(m_StageNumber);
						auto stageGenerator = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator);
						auto stage = stageGenerator->Generate(stageGenerator->GetCodeFromStage(stageNumber));
						auto scene = MakeRef<GameScene>(stage, GameInfo{ GameMode::BlueSpheres, 0, stageNumber });
						m_App->GotoScene(scene);

					}
					
					if (ImGui::Button("Save Stages"))
					{
						for (auto file : Stage::GetStageFiles())
						{
							Stage s;
							s.Load(file);
							s.Save(file);
						}
					}
					
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Shading")) {
					ImGui::SliderFloat("Sky Exposure", &GlobalShadingConfig::SkyExposure, 1.0f, 10.0f);
					ImGui::SliderFloat("Light Radiance", &GlobalShadingConfig::LightRadiance, 0.0f, 10.0f);
					ImGui::SliderFloat("Deferred Exposure", &GlobalShadingConfig::DeferredExposure, 1.0f, 10.0f);
					ImGui::SliderFloat("Bloom Threshold", &GlobalShadingConfig::BloomThreshold, 0.0f, 10.0f);
					ImGui::SliderFloat("Bloom Knee", &GlobalShadingConfig::BloomKnee, 0.0f, 1.0f);
					ImGui::SliderFloat("Bloom Range", &GlobalShadingConfig::BloomRange, 1.0f, 100.0f);
					ImGui::SliderFloat("Ring Emission", &GlobalShadingConfig::RingEmission, 0.0f, 10.0f);
					ImGui::EndTabItem();
				};

				ImGui::EndTabBar();
			}
			ImGui::End();


			int32_t w, h;
			glfwGetWindowSize(m_Window, &w, &h);

			glViewport(0, 0, w, h);
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
	
		void Reset() { Stats.clear(); }

	private:
		Application* m_App = nullptr;
		GLFWwindow* m_Window = nullptr;
		char m_StageNumber[0xff];
	};


	DiagnosticTool& DiagnosticTool::Get()
	{
		static DiagnosticTool instance;
		return instance;
	}

	void DiagnosticTool::Initialize(Application* app, GLFWwindow* window)
	{
		m_Impl->Initialize(app, window);
	}
	
	void DiagnosticTool::Begin()
	{
		m_Impl->Begin();
	}

	void DiagnosticTool::End()
	{
		m_Impl->End();
	}
	
	void DiagnosticTool::Reset()
	{
		m_Impl->Reset();
	}

	DiagnosticTool::DiagnosticTool()
	{
		m_Impl = std::make_unique<Impl>();
	}



	DiagnosticGuard::DiagnosticGuard(const char* name) : m_Name(name)
	{
		m_t0 = Clock::now();
	}

	DiagnosticGuard::~DiagnosticGuard()
	{
		auto& stats = DiagnosticTool::Get().m_Impl->Stats[m_Name];
		auto t1 = Clock::now();
		stats.Calls++;
		stats.ExecutionTime += 
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(t1 - m_t0).count();
	}
}

#endif