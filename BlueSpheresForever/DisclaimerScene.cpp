#include "BsfPch.h"

#include "DisclaimerScene.h"
#include "Application.h"
#include "Renderer2D.h"
#include "Assets.h"
#include "Font.h"


namespace bsf
{

	static std::string s_DisclaimerText = R"(
All the copyrights and registered  trademarks of "Sonic The Hedgehog" and all associated 
characters, art, names, terms and music belong to SEGA®. 

"Blue Spheres Forever" is no way affiliated with SEGA® or Sonic Team®.

"Blue Spheres Forever" is a non-profit project created by fans and no financial gain 
is made from project efforts. No intent to infringe said copyrights or registered trademarks
is made by contributors of "Blue Spheres Forever".

SEGA Mega Drive and Sonic The Hedgehog are trademarks of SEGA Enterprises, LTD.

©1992 SEGA Enterprises, LTD. Character Design ©1991-2012 Sonic Team.
	)";

	void DisclaimerScene::OnAttach()
	{
	}

	void DisclaimerScene::OnRender(const Time& time)
	{

		GLEnableScope scope({ GL_DEPTH_TEST });

		glDisable(GL_DEPTH_TEST);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		auto windowSize = GetApplication().GetWindowSize();
		auto& renderer = GetApplication().GetRenderer2D();
		auto& assets = Assets::GetInstance();
		constexpr float projHeight = 20.0f;
		float projWidth = projHeight * windowSize.x / windowSize.y;

		glViewport(0, 0, windowSize.x, windowSize.y);

		// Render title
		renderer.Begin(glm::ortho(0.0f, projWidth, 0.0f, projHeight, -1.0f, 1.0f));
		{
			renderer.Pivot(EPivot::Top);
			renderer.Push();
			renderer.Translate({ projWidth / 2.0f, projHeight / 4.0f * 3.0f });
			renderer.Color({ 1.0f, 0.0f, 0.0f, 1.0f });
			renderer.DrawString(assets.Get<Font>(AssetName::FontText), "DISCLAIMER");
			renderer.Pop();
		}
		renderer.End();


		// Render text
	}
}