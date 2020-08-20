#include "BsfPch.h"
#include "StageEditorScene.h"

#include "Assets.h"
#include "Texture.h"
#include "Renderer2D.h"
#include "Application.h"
#include "MenuScene.h"

#include <imgui.h>

namespace bsf
{
	static constexpr float s_uiScale = 50.0f;
	static constexpr float s_uiPanelMargin = 1.0f;
	static constexpr float s_uiTopBarHeight = 5.0f;
	static constexpr float s_uiPropertiesWidth = 20.0f;
	static constexpr float s_uiToolbarButtonSize = 4.0f;

	void StageEditorScene::OnAttach()
	{
		m_CurrentStage = MakeRef<Stage>(256, 256);

		for (uint32_t x = 0; x < m_CurrentStage->GetWidth(); x++)
			for (uint32_t y = 0; y < m_CurrentStage->GetHeight(); y++)
				m_CurrentStage->SetValueAt(x, y, EStageObject::BlueSphere);

		InitializeUI();
	}
	
	void StageEditorScene::OnRender(const Time& time)
	{
		auto& app = GetApplication();
		auto& r2 = app.GetRenderer2D();

		const auto windowSize = app.GetWindowSize();

		const auto height = s_uiScale;
		const auto width = windowSize.x / windowSize.y * height;

		GLEnableScope scope({ GL_DEPTH_TEST });

		glViewport(0, 0, windowSize.x, windowSize.y);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		m_uiRoot->Render(windowSize, { width, height }, r2, time);


	}

	void StageEditorScene::OnDetach()
	{
		m_uiRoot->Detach(GetApplication());
	}

	void StageEditorScene::InitializeUI()
	{
		auto& assets = Assets::GetInstance();



		m_uiRoot = MakeRef<UIRoot>();
		m_uiRoot->Attach(GetApplication());

		auto main = MakeRef<UIPanel>();
		main->SetMargin(s_uiPanelMargin);
		main->Layout = UIPanelLayout::Vertical;
		main->BackgroundColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		auto topBar = MakeRef<UIPanel>();
		topBar->Layout = UIPanelLayout::Horizontal;
		topBar->PreferredSize = { 0.0f, s_uiTopBarHeight };

		auto tools = {
			std::make_tuple(StageEditorTool::BlueSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::BlueSphere),
			std::make_tuple(StageEditorTool::RedSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::RedSphere),
			std::make_tuple(StageEditorTool::YellowSphere, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::YellowSphere),
			std::make_tuple(StageEditorTool::Bumper, assets.Get<Texture2D>(AssetName::TexSphereUI), Colors::White),
			std::make_tuple(StageEditorTool::Ring, assets.Get<Texture2D>(AssetName::TexRingUI), Colors::Ring),
		};

		for (const auto& toolSpec : tools)
		{
			auto btn = MakeRef<UIIconButton>();
			
			btn->PreferredSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
			btn->MaxSize = glm::vec2{ s_uiToolbarButtonSize, s_uiToolbarButtonSize };
			btn->Icon = std::get<1>(toolSpec);
			btn->Tint = std::get<2>(toolSpec);
			auto tool = std::get<0>(toolSpec);
			
			AddSubscription(btn->MouseClicked, [&, btn, tool](const MouseEvent& evt) {
				for (auto& b : m_ToolButtons)
					b->Selected = false;
				m_uiEditorArea->ActiveTool = tool;
				btn->Selected = true;
			});

			m_ToolButtons.push_back(btn);
			topBar->AddChild(btn);
		}

		auto middle = MakeRef<UIPanel>();
		middle->Layout = UIPanelLayout::Horizontal;
		middle->PreferredSize = { 0.0f, -s_uiTopBarHeight };

		auto properties = MakeRef<UIPanel>();
		properties->BackgroundColor = { 0.0f, 1.0f, 0.0f, 1.0f };
		properties->PreferredSize = { s_uiPropertiesWidth, 0.0f };

		m_uiEditorArea = MakeRef<UIStageEditorArea>();
		m_uiEditorArea->PreferredSize = { -s_uiPropertiesWidth, 0.0f };
		m_uiEditorArea->SetStage(m_CurrentStage);

		middle->AddChild(m_uiEditorArea);
		middle->AddChild(properties);

		main->AddChild(middle);
		main->AddChild(topBar);


		m_uiRoot->PushLayer(main);
	}


	#pragma region UI Impl


	void UIPanel::SetMargin(float margin)
	{
		Margin.Left = Margin.Right = Margin.Top = Margin.Bottom = margin;
	}

	void UIPanel::AddChild(const Ref<UIElement>& child)
	{
		m_Children.push_back(child);
	}

	void UIPanel::RemoveChild(const Ref<UIElement>& child)
	{

	}

	void UIPanel::Render(Renderer2D& r2, const Time& time)
	{

		r2.Push();
		r2.Color(BackgroundColor);
		r2.DrawQuad(Bounds.Position + glm::vec2(Margin.Left, Margin.Bottom), Bounds.Size - glm::vec2(Margin.Left + Margin.Right, Margin.Top + Margin.Bottom));
		r2.Pop();

		for (auto& child : m_Children)
			child->Render(r2, time);

	}

	void UIPanel::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };

		auto contentSize = Bounds.Size - glm::vec2(Margin.Left + Margin.Right, Margin.Top + Margin.Bottom);

		if (m_Children.size() > 0)
		{
			glm::vec2 origin = Bounds.Position + glm::vec2{ Margin.Left, Margin.Right };

			if (Layout == UIPanelLayout::Vertical)
			{
				// Resize children
				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						std::max(child->MinSize.x, contentSize.x),
						child->PreferredSize.y > 0 ? child->PreferredSize.y : contentSize.y + child->PreferredSize.y
					};
					child->UpdateBounds(origin, computedSize);

					origin.y += computedSize.y;

				}
			}
			else if (Layout == UIPanelLayout::Horizontal)
			{

				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						child->PreferredSize.x > 0 ? child->PreferredSize.x : contentSize.x + child->PreferredSize.x,
						//std::max(child->MinSize.y, contentSize.y)
						std::clamp(contentSize.y, child->MinSize.y, child->MaxSize.y)
					};

					child->UpdateBounds(origin + glm::vec2(0.0f, (contentSize.y - computedSize.y) / 2.0f), computedSize);
					origin.x += computedSize.x;
				}
			}
			else if (Layout == UIPanelLayout::Fill)
			{
				auto& child = m_Children.front();
				
				glm::vec2 computedSize = {
					std::clamp(contentSize.x, child->MinSize.x, child->MaxSize.x),
					std::clamp(contentSize.y, child->MinSize.y, child->MaxSize.y)
				};

				child->UpdateBounds(origin + glm::vec2(0.0f, (contentSize.y - computedSize.y) / 2.0f), computedSize);
			}
			else
			{

				for (auto& child : m_Children)
				{
					child->UpdateBounds(origin + child->Position, child->PreferredSize);
				}
			}
		}


	}

	uint32_t UIElement::m_NextId = 1;

	UIElement::UIElement()
	{
		m_Id = m_NextId++;
	}

	const std::vector<Ref<UIElement>>& UIElement::Children()
	{
		static const std::vector<Ref<UIElement>> s_Empty;
		return s_Empty;
	}

	void UIElement::Traverse(const std::function<void(UIElement&)>& action)
	{
		for (auto& child : Children())
		{
			child->Traverse(action);
		}
		action(*this);
	}


	void UIRoot::Attach(Application& app)
	{
		AddSubscription(app.MouseMoved, [&](const MouseEvent& evt) {
			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.PrevPosition = m_MouseState.Position;
			m_MouseState.Position = pos;
			m_MouseState.HoverTarget = nullptr;

			m_Layers.back()->Traverse([](UIElement& element) { element.Hovered = false; });

			auto intersection = Intersect(m_Layers.back(), pos);
			if (intersection)
			{

				m_MouseState.HoverTarget = intersection;

				intersection->Hovered = true;
				intersection->MouseMoved.Emit({ pos.x, pos.y, 0, 0, MouseButton::None });

				auto& leftButtonState = m_MouseButtonState[MouseButton::Left];
				if (intersection == m_MouseState.DragTarget)
				{
					auto delta = m_MouseState.Position - m_MouseState.PrevPosition;
					intersection->MouseDragged.Emit({ pos.x, pos.y, delta.x, delta.y, m_MouseState.DragButton });
				}

			}

		});

		AddSubscription(app.Wheel, [&](const WheelEvent& evt) {
			if (m_MouseState.HoverTarget)
				m_MouseState.HoverTarget->Wheel.Emit(evt);
		});

		AddSubscription(app.MousePressed, [&](const MouseEvent& evt) {
			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.Position = pos;
			m_MouseState.PrevPosition = pos;

			auto intersection = Intersect(m_Layers.back(), pos);
			if (intersection)
			{
				auto& buttonState = m_MouseButtonState[evt.Button];
				buttonState.Pressed = true;
				buttonState.TimePressed = std::chrono::system_clock::now();
				m_MouseState.DragTarget = intersection;
				m_MouseState.DragButton = evt.Button;
				//intersection->MousePressed.Emit({ pos.x, pos.y, evt.Button });
			}
			else
			{
				m_MouseState.DragTarget = nullptr;
			}
		});

		AddSubscription(app.MouseReleased, [&](const MouseEvent& evt) {
			if (m_Layers.empty())
				return;

			glm::vec2 pos = GetMousePosition({ evt.X,evt.Y });

			m_MouseState.DragTarget = nullptr;
			m_MouseState.Position = pos;
			m_MouseState.PrevPosition = pos;

			auto intersection = Intersect(m_Layers.back(), pos);


			if (intersection)
			{
				auto& buttonState = m_MouseButtonState[evt.Button];
				buttonState.Pressed = false;

				if (std::chrono::system_clock::now() - buttonState.TimePressed < std::chrono::milliseconds(500))
				{
					intersection->MouseClicked.Emit({ pos.x, pos.y, 0, 0, evt.Button });
				}
			}
		});
	}

	void UIRoot::Detach(Application& app)
	{
		for (auto& layer : m_Layers)
			layer->Traverse([](UIElement& el) { el.ClearSubscriptions(); });
		ClearSubscriptions();
	}

	void UIRoot::Render(const glm::vec2& windowSize, const glm::vec2& viewport, Renderer2D& r2, const Time& time)
	{
		m_WindowSize = windowSize;
		m_Viewport = viewport;
		m_Projection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
		m_InverseProjection = glm::inverse(m_Projection);
		r2.Begin(m_Projection);
		for (auto& l = m_Layers.rbegin(); l != m_Layers.rend(); l++)
		{
			(*l)->UpdateBounds({ 0.0f, 0.0f }, viewport);
			(*l)->Render(r2, time);
		}
		r2.End();
	}

	glm::vec2 UIRoot::GetMousePosition(const glm::vec2& windowMousePos) const
	{
		glm::vec4 ndc = { windowMousePos.x / m_WindowSize.x * 2.0 - 1.0f, 1.0f - windowMousePos.y / m_WindowSize.y * 2.0f, 0.0f, 1.0f };
		return m_InverseProjection * ndc;
	}

	Ref<UIElement> UIRoot::Intersect(const Ref<UIElement>& root, const glm::vec2& pos, const std::function<bool(const UIElement&)>& predicate)
	{
		static auto defaultPredicate = [](const auto&) -> bool {return true; };

		for (const auto& child : root->Children())
		{
			auto result = Intersect(child, pos, predicate);
			if (result != nullptr)
				return result;
		}

		return root->Bounds.Contains(pos) && (predicate ? predicate : defaultPredicate)(*root) ? root : nullptr;
	}



	void UIIconButton::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIIconButton::Render(Renderer2D& r2, const Time& time)
	{
		static constexpr float s_ShadowOffset = 0.2f;
		r2.Push();
		r2.Texture(Icon);

		r2.Push();
		{
			r2.Translate({ s_ShadowOffset, -s_ShadowOffset });
			r2.Color(UIDefaultStyle::ShadowColor);
			r2.DrawQuad(Bounds.Position, Bounds.Size);
		}
		r2.Pop();

		r2.Color((Hovered || Selected) ? Tint : UIDefaultStyle::IconButtonDefaultTint * Tint);
		r2.DrawQuad(Bounds.Position, Bounds.Size);
		r2.Pop();
	}


	UIStageEditorArea::UIStageEditorArea()
	{
		auto& assets = Assets::GetInstance();

		static const std::unordered_map<StageEditorTool, EStageObject> s_toolMap = {
			{ StageEditorTool::BlueSphere, EStageObject::BlueSphere },
			{ StageEditorTool::RedSphere, EStageObject::RedSphere },
			{ StageEditorTool::YellowSphere, EStageObject::YellowSphere },
			{ StageEditorTool::Bumper, EStageObject::Bumper },
			{ StageEditorTool::Ring, EStageObject::Ring }
		};

		m_StageObjRendering = {
			{ EStageObject::BlueSphere,		{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::BlueSphere }},
			{ EStageObject::RedSphere,		{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::RedSphere }},
			{ EStageObject::YellowSphere,	{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::YellowSphere }},
			{ EStageObject::Bumper,			{ assets.Get<Texture2D>(AssetName::TexSphereUI),	Colors::White }},
			{ EStageObject::Ring,			{ assets.Get<Texture2D>(AssetName::TexRingUI),		Colors::Ring }}
		};

		m_Pattern = CreateCheckerBoard({ 0, 0 });

		auto editCallback = [&](const MouseEvent& evt) {

			if (auto stageCoordsOpt = GetStageCoordinates({ evt.X, evt.Y }); stageCoordsOpt.has_value())
			{
				const auto& stageCoords = stageCoordsOpt.value();

				if (evt.Button == MouseButton::Left)
				{
					if (auto obj = s_toolMap.find(ActiveTool); obj != s_toolMap.end())
						m_Stage->SetValueAt(stageCoords.x, stageCoords.y, obj->second);
				}

				if (evt.Button == MouseButton::Right)
				{
					m_Stage->SetValueAt(stageCoords.x, stageCoords.y, EStageObject::None);
					m_Stage->SetAvoidSearchAt(stageCoords.x, stageCoords.y, EAvoidSearch::No);
				}
			}
		};

		AddSubscription(MouseClicked, editCallback);
		AddSubscription(MouseDragged, editCallback);
		AddSubscription(MouseDragged, [&](const MouseEvent& evt) {
			if (evt.Button == MouseButton::Middle)
				m_ViewOrigin -= glm::vec2(evt.DeltaX, evt.DeltaY);
		});


		AddSubscription(Wheel, [&](const WheelEvent& evt) {
			m_Zoom = std::clamp(m_Zoom + evt.DeltaY * 0.2f, m_MinZoom, m_MaxZoom);
		});

	}

	void UIStageEditorArea::Render(Renderer2D& r2, const Time& time)
	{

		if (m_Stage)
		{

			auto size = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) * m_Zoom;
			auto tiling = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) / 2.0f;

			r2.Push();
			
			r2.Color({ 0.0f, 0.0f, 0.0f, 0.25f });
			r2.DrawQuad(Bounds.Position, Bounds.Size);

			r2.Clip(Bounds);

			r2.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
			r2.Texture(m_Pattern);
			r2.DrawQuad(Bounds.Position - m_ViewOrigin, size, tiling, { -0.25f, -0.25f });

			r2.Translate(Bounds.Position - m_ViewOrigin);
			r2.Scale({ m_Zoom, m_Zoom });

			for (uint32_t x = 0; x < m_Stage->GetWidth(); ++x)
			{
				for (uint32_t y = 0; y < m_Stage->GetHeight(); ++y)
				{
					if (auto obj = m_StageObjRendering.find(m_Stage->GetValueAt(x, y)); obj != m_StageObjRendering.end())
					{
						r2.Texture(std::get<0>(obj->second));

						r2.Color(UIDefaultStyle::ShadowColor);
						r2.DrawQuad({ x + 0.1f, y - 0.1f });
						
						r2.Color(std::get<1>(obj->second));
						r2.DrawQuad({ x, y });
					}
				}
			}
			
			r2.Pop();
		}
	}

	void UIStageEditorArea::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIStageEditorArea::UpdatePattern()
	{
		if (m_Stage)
		{
			m_Pattern = CreateCheckerBoard({
				ToHexColor(m_Stage->CheckerColors[0]),
				ToHexColor(m_Stage->CheckerColors[1])
				});
		}

	}

	std::optional<glm::ivec2> UIStageEditorArea::GetStageCoordinates(const glm::vec2 pos) const
	{
		glm::ivec2 localPos = glm::floor(((pos - Bounds.Position) + m_ViewOrigin) / m_Zoom);

		if (m_Stage != nullptr && localPos.x >= 0 && localPos.x < m_Stage->GetWidth() 
			&& localPos.y >= 0 && localPos.y < m_Stage->GetHeight())
			return localPos;
		else
			return std::nullopt;

	}

	void UITextInput::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UITextInput::Render(Renderer2D& r2, const Time& time)
	{
		r2.Push();

		r2.Pop();
	}

	#pragma endregion







}