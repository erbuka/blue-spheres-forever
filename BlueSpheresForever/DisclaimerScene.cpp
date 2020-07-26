#include "BsfPch.h"

#include "DisclaimerScene.h"
#include "Application.h"
#include "Renderer2D.h"
#include "Assets.h"
#include "Font.h"
#include "Common.h"
#include "SplashScene.h"

namespace bsf
{

	static const std::array<std::string, 11> s_DisclaimerText = {
		"All the copyrights and registered  trademarks of \"Sonic The Hedgehog\" and all associated",
		"characters, art, names, termsand music belong to SEGA.",
		"",
		"\"Blue Spheres Forever\" is no way affiliated with SEGA or Sonic Team.",
		"",
		"\"Blue Spheres Forever\" is a non - profit project created by fans and no financial gain is",
		"made from project efforts. No intent to infringe said copyrights or registered trademarks",
		"is made by contributors of \"Blue Spheres Forever\".",
		"",
		"SEGA Mega Driveand Sonic The Hedgehog are trademarks of SEGA Enterprises, LTD.",
		"Copyright 1992 SEGA Enterprises, LTD.Character Design 1991 - 2012 Sonic Team."
	};



	static constexpr float s_Width = 50.0f;

	void DisclaimerScene::OnAttach()
	{
		auto fadeIn = MakeRef<FadeTask>(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 0.5f);
		ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeIn);
		
		auto waitFadeOut = MakeRef<WaitForTask>(5.0f);

		waitFadeOut->SetDoneFunction([&](SceneTask& self) {
			auto fadeOut = MakeRef<FadeTask>(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.5f);
			fadeOut->SetDoneFunction([&](SceneTask& self) {
				GetApplication().GotoScene(MakeRef<SplashScene>());
			});
			ScheduleTask<FadeTask>(ESceneTaskEvent::PostRender, fadeOut);
		});

		ScheduleTask<WaitForTask>(ESceneTaskEvent::PostRender, waitFadeOut);

	}

	void DisclaimerScene::OnRender(const Time& time)
	{
		auto windowSize = GetApplication().GetWindowSize();
		auto& renderer = GetApplication().GetRenderer2D();
		auto& assets = Assets::GetInstance();
		auto textFont = assets.Get<Font>(AssetName::FontText);

		float length = 0.0f;
		for (const auto& s : s_DisclaimerText)
			length = std::max(length, textFont->GetStringWidth(s));

		float padding = (s_Width - length) / 2.0f;

		float projWidth = s_Width;
		float projHeight = s_Width / windowSize.x * windowSize.y;

		// Compute the max row length
		

		GLEnableScope scope({ GL_DEPTH_TEST });

		glDisable(GL_DEPTH_TEST);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		glViewport(0, 0, windowSize.x, windowSize.y);

		// Render title
		renderer.Begin(glm::ortho(0.0f, projWidth, 0.0f, projHeight, -1.0f, 1.0f));
		{

			renderer.Translate({ 0.0f, projHeight / 4.0f * 3.0f });

			renderer.Pivot(EPivot::Bottom);

			renderer.Push();
			renderer.Translate({ projWidth  / 2.0f, 0.0f });
			renderer.Color({ 1.0f, 0.0f, 0.0f, 1.0f });
			renderer.DrawString(assets.Get<Font>(AssetName::FontText), "DISCLAIMER");
			renderer.Pop();

			renderer.Pivot(EPivot::TopLeft);

			renderer.Translate({ padding, -1.0f });
			renderer.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
			for (const auto& s : s_DisclaimerText)
			{
				renderer.DrawString(textFont, s);
				renderer.Translate({ 0.0f, -1.0f });
			}

		}
		renderer.End();


		// Render text
	}
}