#include "BsfPch.h"
#include "StageClearScene.h"

#include "Application.h"
#include "Renderer2D.h"
#include "Assets.h"
#include "Font.h"
#include "Stage.h"
#include "GameScene.h"
#include "SplashScene.h"
#include "Audio.h"

namespace bsf
{
	static constexpr uint32_t s_PerfectBonus = 50000;
	static constexpr uint32_t s_RingBonus = 100;

	StageClearScene::StageClearScene(const GameInfo& gameInfo, uint32_t collectedRings, bool perfect) :
		Scene(),
		m_GameInfo(gameInfo)
	{
		m_CurrentStage = gameInfo.CurrentStage;
		m_NextStage = gameInfo.CurrentStage + (perfect ? 10 : 1);

		m_RingBonus.Reset(collectedRings * s_RingBonus, 0.0f);
		m_PerfectBonus.Reset(perfect * s_PerfectBonus, 0.0f);
		m_Score.Reset(gameInfo.Score, gameInfo.Score + (uint32_t)m_RingBonus + (uint32_t)m_PerfectBonus);
	}

	void StageClearScene::OnAttach()
	{
		auto& app = GetApplication();

		if(m_GameInfo.Mode == GameMode::BlueSpheres)
			m_CurrentStageCode = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator)->GetCodeFromStage(m_CurrentStage);

		// Play sound
		Assets::GetInstance().Get<Audio>(AssetName::SfxStageClear)->Play();

		// Some call backs

		// Tasks
		auto fadeIn = MakeRef<FadeTask>(glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.0f }, 0.5f);
		ScheduleTask(ESceneTaskEvent::PostRender, fadeIn);


		auto waitForUpdate = MakeRef<WaitForTask>(3.0f);
		waitForUpdate->SetDoneFunction([&](SceneTask& self) {

			auto updateScore = MakeRef<SceneTask>();
			updateScore->SetUpdateFunction([&](SceneTask& self, const Time& time) {
				constexpr float duration = 3.0f;

				float delta = std::min(1.0f, (time.Elapsed - self.GetStartTime().Elapsed) / duration);

				m_RingBonus(delta);
				m_PerfectBonus(delta);
				m_Score(delta);

				if (delta == 1.0f)
				{
					Assets::GetInstance().Get<Audio>(AssetName::SfxTally)->Play();
					m_InputEnabled = true;
					self.SetDone();
				}

			});
			ScheduleTask(ESceneTaskEvent::PostRender, updateScore);
		});
		ScheduleTask(ESceneTaskEvent::PostRender, waitForUpdate);


		// Input
		m_Subscriptions.push_back(app.KeyReleased.Subscribe([&](const KeyReleasedEvent& evt) {
			
			if (m_InputEnabled && evt.KeyCode == GLFW_KEY_ENTER)
			{
				auto fadeOut = MakeRef<FadeTask>(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.5f);

				fadeOut->SetDoneFunction([&](SceneTask& self) {
					if (m_GameInfo.Mode == GameMode::BlueSpheres)
					{
						auto& stageGenerator = Assets::GetInstance().Get<StageGenerator>(AssetName::StageGenerator);
						auto newGameInfo = m_GameInfo;
						newGameInfo.CurrentStage = m_NextStage;
						newGameInfo.Score = m_Score.Get<1>();
						auto stage = stageGenerator->Generate(stageGenerator->GetCodeFromStage(newGameInfo.CurrentStage));
						auto scene = MakeRef<GameScene>(stage, newGameInfo);
						GetApplication().GotoScene(scene);
					}
					else
					{
						GetApplication().GotoScene(MakeRef<SplashScene>());
					}
				});

				ScheduleTask(ESceneTaskEvent::PostRender, fadeOut);

			}
		}));

	}

	void StageClearScene::OnRender(const Time& time)
	{
		auto& app = GetApplication();
		auto windowSize = app.GetWindowSize();

		constexpr float height = 10.0f;
		float width = windowSize.x / windowSize.y * height;

		glViewport(0, 0, windowSize.x, windowSize.y);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderUI();
	}
	void StageClearScene::OnDetach()
	{
		for (auto& unsubscribe : m_Subscriptions)
			unsubscribe();
	}
	void StageClearScene::RenderUI()
	{
		auto& assets = Assets::GetInstance();
		auto& app = GetApplication();
		auto& r2 = app.GetRenderer2D();
		auto font = assets.Get<Font>(AssetName::FontMain);
		auto windowSize = app.GetWindowSize();

		constexpr float height = 15.0f;
		float width = windowSize.x / windowSize.y * height;


		auto drawCounter = [&](const std::string& str, uint64_t count) {

			char buffer[32];

			r2.Push();

			r2.Translate({ width / 4.0f, 0.0f });
			r2.Pivot(EPivot::TopLeft);
			r2.DrawStringShadow(font, str);
			
			r2.Translate({ width / 2.0f, 0.0f });
			r2.Pivot(EPivot::TopRight);
			r2.DrawStringShadow(font, std::to_string(count));

			r2.Pop();
		};

		r2.Begin(glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f));
		r2.Color(Colors::Black);
		r2.TextShadowColor({ 0.0f, 0.0f, 0.0f, 0.25f });
		r2.TextShadowOffset({ 0.02f, -0.02f });

		r2.Push();
		{
			r2.Translate({ width / 2.0f, height });
			r2.Scale({ 2.0f, 2.0f });
			r2.Pivot(EPivot::Top);
			r2.DrawStringShadow(font, "Stage Clear");

			if (m_GameInfo.Mode == GameMode::BlueSpheres)
			{
				r2.Translate({ 0.0f, -1.0f });
				r2.Scale({ 0.5f, 0.5f });
				r2.DrawStringShadow(font, (std::string)m_CurrentStageCode);
			}
		}
		r2.Pop();

		r2.Push();
		{
			r2.Translate({ 0.0f, height / 2.0f });
			drawCounter("Ring Bonus", (uint32_t)m_RingBonus);

			r2.Translate({ 0.0f, -1.0f });
			drawCounter("Perfect Bonus", (uint32_t)m_PerfectBonus);

			r2.Translate({ 0.0f, -2.0f });
			drawCounter("Score", (uint64_t)m_Score);

		}

		r2.End();
	}
}