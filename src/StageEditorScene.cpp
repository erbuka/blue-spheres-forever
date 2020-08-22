#include "BsfPch.h"
#include "StageEditorScene.h"

#include <regex>

#include "Assets.h"
#include "Texture.h"
#include "Renderer2D.h"
#include "Application.h"
#include "MenuScene.h"
#include "Font.h"

#include <imgui.h>

namespace bsf
{
	static constexpr float s_uiScale = 15.0f;
	static constexpr float s_uiPanelMargin = s_uiScale / 100.0f;
	static constexpr float s_uiTopBarHeight = s_uiScale / 10.0f;
	static constexpr float s_uiPropertiesWidth = s_uiScale / 3.0f;
	static constexpr float s_uiToolbarButtonSize = s_uiTopBarHeight - 2 * s_uiPanelMargin;

	void StageEditorScene::OnAttach()
	{
		m_CurrentStage = MakeRef<Stage>();
		/*
		for (uint32_t x = 0; x < m_CurrentStage->GetWidth(); x++)
			for (uint32_t y = 0; y < m_CurrentStage->GetHeight(); y++)
				m_CurrentStage->SetValueAt(x, y, EStageObject::BlueSphere);
		*/
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

		UIStyle style;
		style.GlobalScale = s_uiScale;

		m_uiRoot = MakeRef<UIRoot>();
		m_uiRoot->Attach(GetApplication());
		m_uiRoot->SetStyle(style);




		// Top Bar
		auto topBar = MakeRef<UIPanel>();
		topBar->Layout = UIPanelLayout::Horizontal;
		topBar->PreferredSize = { 0.0f, s_uiTopBarHeight };

		auto title = MakeRef<UIText>();
		title->Text = "Stage Editor";
		topBar->AddChild(title);

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

		// Properties
		auto properties = MakeRef<UIPanel>();
		properties->Layout = UIPanelLayout::Vertical;
		properties->PreferredSize = { s_uiPropertiesWidth, 0.0f };

		// Max Rings input
		{ 
			auto maxRingsInput = MakeRef<UITextInput>();
			maxRingsInput->Label = "Max Rings";
			maxRingsInput->BindPush([&, check = std::regex("^[0-9]+$")](const std::string& val) {

				BSF_INFO("{0}", val);

				if (std::regex_match(val, check))
				{
					m_CurrentStage->MaxRings = std::atoi(val.c_str());
					return true;
				}
				else if (val.empty())
				{
					m_CurrentStage->MaxRings = 0;
					return true;
				}
				else
				{
					return false;
				}
			});

			maxRingsInput->BindPull([&] {return std::to_string(m_CurrentStage->MaxRings); });

			properties->AddChild(maxRingsInput);
		}


		// Editor area
		m_uiEditorArea = MakeRef<UIStageEditorArea>();
		m_uiEditorArea->PreferredSize = { -s_uiPropertiesWidth, 0.0f };
		m_uiEditorArea->SetStage(m_CurrentStage);

		// Middle section
		auto middle = MakeRef<UIPanel>();
		middle->Layout = UIPanelLayout::Horizontal;
		middle->PreferredSize = { 0.0f, -s_uiTopBarHeight };

		middle->AddChild(m_uiEditorArea);
		middle->AddChild(properties);

		// Main panel
		auto main = MakeRef<UIPanel>();
		main->SetMargin(s_uiPanelMargin);
		main->Layout = UIPanelLayout::Vertical;

		main->AddChild(topBar);
		main->AddChild(middle);

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
		r2.Color(GetBackgroundColor());
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

			if (Layout == UIPanelLayout::Vertical)
			{
				glm::vec2 origin = Bounds.Position + glm::vec2{ Margin.Left, Margin.Bottom } + glm::vec2(0.0f, contentSize.y);

				// Resize children
				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						std::clamp(contentSize.x, child->MinSize.x, child->MaxSize.x),
						child->PreferredSize.y > 0 ? child->PreferredSize.y : contentSize.y + child->PreferredSize.y
					};
					
					origin.y -= computedSize.y;

					child->UpdateBounds(origin, computedSize);


				}
			}
			else if (Layout == UIPanelLayout::Horizontal)
			{
				glm::vec2 origin = Bounds.Position + glm::vec2{ Margin.Left, Margin.Right };

				for (auto& child : m_Children)
				{
					glm::vec2 computedSize = {
						child->PreferredSize.x > 0 ? child->PreferredSize.x : contentSize.x + child->PreferredSize.x,
						std::clamp(contentSize.y, child->MinSize.y, child->MaxSize.y)
					};

					child->UpdateBounds(origin + glm::vec2(0.0f, (contentSize.y - computedSize.y) / 2.0f), computedSize);
					origin.x += computedSize.x;
				}
			}
			else if (Layout == UIPanelLayout::Fill)
			{
				glm::vec2 origin = Bounds.Position + glm::vec2{ Margin.Left, Margin.Right };

				auto& child = m_Children.front();
				
				glm::vec2 computedSize = {
					std::clamp(contentSize.x, child->MinSize.x, child->MaxSize.x),
					std::clamp(contentSize.y, child->MinSize.y, child->MaxSize.y)
				};

				child->UpdateBounds(origin + glm::vec2(0.0f, (contentSize.y - computedSize.y) / 2.0f), computedSize);
			}
			else
			{

				glm::vec2 origin = Bounds.Position + glm::vec2{ Margin.Left, Margin.Right };


				for (auto& child : m_Children)
				{
					child->UpdateBounds(origin + child->Position, child->PreferredSize);
				}
			}
		}


	}

	uint32_t UIElement::m_NextId = 1;

	UIElement::UIElement() : UIElement(MakeFlags(UIElementFlags::None))
	{
	}

	UIElement::UIElement(std::underlying_type_t<UIElementFlags> flags) :
		m_Flags(flags),
		m_Id(m_NextId++)
	{
	}

	bool UIElement::GetFlag(UIElementFlags flag) const
	{
		return m_Flags && static_cast<std::underlying_type_t<UIElementFlags>>(flag);
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


	void UIRoot::SetStyle(const UIStyle& style)
	{
		m_Style = style;
		m_Style.Recompute();
	}

	void UIRoot::Attach(Application& app)
	{
		AddSubscription(app.KeyPressed, [&](const KeyPressedEvent& evt) {
			if (m_FocusedControl)
				m_FocusedControl->KeyPressed.Emit(evt);
		});

		AddSubscription(app.KeyReleased, [&](const KeyReleasedEvent& evt) {
			if (m_FocusedControl)
				m_FocusedControl->KeyReleased.Emit(evt);
		});

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

				intersection->MouseMoved.Emit({ pos.x, pos.y, 0, 0, MouseButton::None });
				
				m_MouseState.HoverTarget = intersection;

				if (intersection->GetFlag(UIElementFlags::ReceiveHover))
					intersection->Hovered = true;

	

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

			auto& buttonState = m_MouseButtonState[evt.Button];
			buttonState.Pressed = true;
			buttonState.TimePressed = std::chrono::system_clock::now();

			auto intersection = Intersect(m_Layers.back(), pos);

			m_Layers.back()->Traverse([](UIElement& el) {el.Focused = false; });
			m_FocusedControl = nullptr;

			if (intersection)
			{
				m_MouseState.DragTarget = intersection;
				m_MouseState.DragButton = evt.Button;

				if (intersection->GetFlag(UIElementFlags::ReceiveFocus))
				{
					intersection->Focused = true;
					m_FocusedControl = intersection;
				}

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
			auto& buttonState = m_MouseButtonState[evt.Button];
			buttonState.Pressed = false;


			if (intersection)
			{

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
			(*l)->Traverse([&](UIElement& el) { el.m_Style = &m_Style; el.Update(time); });
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

		r2.Push();
		r2.Texture(Icon);

		r2.Push();
		{
			r2.Translate({ GetStyle().IconButtonShadowOffset, -GetStyle().IconButtonShadowOffset });
			r2.Color(GetStyle().ShadowColor);
			r2.DrawQuad(Bounds.Position, Bounds.Size);
		}
		r2.Pop();

		r2.Color((Hovered || Selected) ? Tint : GetStyle().IconButtonDefaultTint * Tint);
		r2.DrawQuad(Bounds.Position, Bounds.Size);
		r2.Pop();
	}


	UIStageEditorArea::UIStageEditorArea() : UIElement(MakeFlags(UIElementFlags::ReceiveHover))
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
		m_BgPattern = CreateCheckerBoard({ 0xffffffff, 0xffcccccc });

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

		AddSubscription(MouseMoved, [&](const MouseEvent& evt) {
			m_CursorPos = GetStageCoordinates({ evt.X,evt.Y });
		});
		AddSubscription(MouseClicked, editCallback);
		AddSubscription(MouseDragged, editCallback);
		AddSubscription(MouseDragged, [&](const MouseEvent& evt) {
			if (evt.Button == MouseButton::Middle)
				m_ViewOrigin -= glm::vec2(evt.DeltaX, evt.DeltaY);
		});


		AddSubscription(Wheel, [&](const WheelEvent& evt) {
			const float m = evt.DeltaY > 0 ? 1.5f : (1.0f / 1.5f);
			m_TargetZoom = std::clamp(m_TargetZoom * m, m_MinZoom, m_MaxZoom);
		});


	}

	void UIStageEditorArea::Update(const Time& time)
	{
		float zoomSpeed = std::max(0.5f, std::abs(m_Zoom - m_TargetZoom)) * 4.0f;
		m_Zoom = MoveTowards(m_Zoom, m_TargetZoom, time.Delta * zoomSpeed);
	}

	void UIStageEditorArea::Render(Renderer2D& r2, const Time& time)
	{

		if (m_Stage)
		{

			auto size = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) * m_Zoom;
			auto tiling = glm::vec2(m_Stage->GetWidth(), m_Stage->GetHeight()) / 2.0f;

			r2.Push();
			
			r2.Color(GetBackgroundColor());
			r2.DrawQuad(Bounds.Position, Bounds.Size);

			r2.Clip(Bounds);
			
			r2.Texture(m_BgPattern);
			r2.DrawQuad(Bounds.Position, Bounds.Size, Bounds.Size / (m_Zoom * 2.0f) , m_ViewOrigin / (m_Zoom * 2.0f) + glm::vec2(0.25f, 0.25f));

			r2.Color({ 1.0f, 1.0f, 1.0f, 1.0f });
			r2.Texture(m_Pattern);
			r2.DrawQuad(Bounds.Position - m_ViewOrigin, size, tiling, { -0.25f, -0.25f });

			r2.Translate(Bounds.Position - m_ViewOrigin);
			r2.Scale({ m_Zoom, m_Zoom });

			glm::uvec2 bl = glm::clamp(glm::floor(GetRawStageCoordinates(Bounds.Position)), 0.0f, (float)m_Stage->GetWidth());
			glm::uvec2 tr = glm::clamp(glm::ceil(GetRawStageCoordinates(Bounds.Position + Bounds.Size)), 0.0f, (float)m_Stage->GetHeight());

			for (uint32_t x = bl.x; x < tr.x; ++x)
			{
				for (uint32_t y = bl.y; y < tr.y; ++y)
				{

					if (auto obj = m_StageObjRendering.find(m_Stage->GetValueAt(x, y)); obj != m_StageObjRendering.end())
					{
						r2.Texture(std::get<0>(obj->second));

						r2.Color(GetStyle().ShadowColor);
						r2.DrawQuad({ x + GetStyle().StageAreaShadowOffset, y - GetStyle().StageAreaShadowOffset });
						
						r2.Color(std::get<1>(obj->second));
						r2.DrawQuad({ x, y });
					}
				}
			}

			DrawCursor(r2);

			
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

	void UIStageEditorArea::DrawCursor(Renderer2D& r2)
	{
		static const float lw = GetStyle().StageAreaCrosshairSize; // line width
		static const float lt = GetStyle().StageAreaCrosshairThickness; // thickness
		static const std::array<std::pair<glm::vec2, glm::vec2>, 8> quads = {
			std::make_pair(glm::vec2(0, 0), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(0, 0), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(1.0f - lw, 0), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(1.0f - lt, 0), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(1.0f - lw, 1.0f - lt), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(1.0f - lt, 1.0f - lw), glm::vec2(lt, lw)),
			std::make_pair(glm::vec2(0, 1.0f - lt), glm::vec2(lw, lt)),
			std::make_pair(glm::vec2(0, 1.0f - lw), glm::vec2(lt, lw)),
		};

		if (Hovered && m_CursorPos != std::nullopt)
		{
			r2.Push();
			r2.Translate(m_CursorPos.value());
			r2.NoTexture();
			{ // Shadow
				r2.Push();
				r2.Translate({ GetStyle().StageAreaShadowOffset, -GetStyle().StageAreaShadowOffset });
				r2.Color(Colors::Black);
				for (const auto& [pos, size] : quads)
					r2.DrawQuad(pos, size);
				r2.Pop();
			}

			r2.Color(GetStyle().Palette.Secondary);
			for (const auto& [pos, size] : quads)
				r2.DrawQuad(pos, size);

			r2.Pop();
		}
	}

	glm::vec2 UIStageEditorArea::GetRawStageCoordinates(const glm::vec2 pos) const
	{
		return  (pos - Bounds.Position + m_ViewOrigin) / m_Zoom;
	}

	std::optional<glm::ivec2> UIStageEditorArea::GetStageCoordinates(const glm::vec2 pos) const
	{
		glm::ivec2 localPos = glm::floor(GetRawStageCoordinates(pos));

		if (m_Stage != nullptr && localPos.x >= 0 && localPos.x < m_Stage->GetWidth() 
			&& localPos.y >= 0 && localPos.y < m_Stage->GetHeight())
			return localPos;
		else
			return std::nullopt;

	}

	UITextInput::UITextInput() : UIElement(MakeFlags(UIElementFlags::ReceiveFocus))
	{
		MinSize = { 0.0f, 1.5f };
		PreferredSize = { 0.0f, 1.5f };

		AddSubscription(KeyPressed, [&](const KeyPressedEvent& evt) {
			if ((evt.KeyCode >= GLFW_KEY_A && evt.KeyCode <= GLFW_KEY_Z) ||
				(evt.KeyCode >= GLFW_KEY_0 && evt.KeyCode <= GLFW_KEY_9))
			{
				m_Push(m_Value + (char)evt.KeyCode);
			}
			else if (evt.KeyCode == GLFW_KEY_BACKSPACE && !m_Value.empty())
			{
				m_Push(m_Value.substr(0, m_Value.size() - 1));
			}
		});

	}

	void UITextInput::Update(const Time& time)
	{
		// Update the current value
		m_Value = std::move(m_Pull());
	}

	void UITextInput::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UITextInput::Render(Renderer2D& r2, const Time& time)
	{

		const float margin = GetStyle().ComputeMargin(GetStyle().TextInputMargin);
		Rect innerBounds = Bounds;
		glm::vec2 shadowOffset = { GetStyle().TextInputShadowOffset, -GetStyle().TextInputShadowOffset };

		innerBounds.Position += glm::vec2(margin);
		innerBounds.Size -= glm::vec2(margin *  2.0f);


		r2.Push();

		r2.Color(GetBackgroundColor());
		r2.DrawQuad(innerBounds.Position, innerBounds.Size);

		r2.TextShadowColor(GetStyle().ShadowColor);
		r2.TextShadowOffset(shadowOffset);

		r2.Translate(innerBounds.Position);

		r2.Color(Focused ? GetStyle().Palette.Secondary : GetForegroundColor());

		// Text
		{
			r2.Push();

			r2.Pivot(EPivot::Center);

			r2.Translate(innerBounds.Size / 2.0f);
			r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), m_Value);

			r2.Pop();
		}

		// Label

		{
			r2.Push();

			r2.Pivot(EPivot::BottomLeft);

			r2.Translate({ margin, 0.8f });
			r2.Scale({ GetStyle().TextInputLabelFontScale, GetStyle().TextInputLabelFontScale });
			r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), Label);
			
			r2.Pop();
		}


		r2.Pop();
	}

	#pragma endregion



	void UIStyle::Recompute()
	{
		MarginUnit = GlobalScale / 100.0f;
		IconButtonShadowOffset = GlobalScale / 200.0f;
	}

	void UIText::Update(const Time& time)
	{
		auto font = Assets::GetInstance().Get<Font>(AssetName::FontMain);
		const float margin = GetStyle().ComputeMargin(GetStyle().TextMargin);
		float width = font->GetStringWidth(Text);
		PreferredSize = MinSize = { width + 2.0f * margin, 1.0f };
	}

	void UIText::UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize)
	{
		Bounds = { origin, computedSize };
	}

	void UIText::Render(Renderer2D& r2, const Time& time)
	{
		const float margin = GetStyle().ComputeMargin(GetStyle().TextMargin);
		auto contentBounds = Bounds;
		contentBounds.Position += glm::vec2(margin);
		contentBounds.Size -= 2.0f * glm::vec2(margin);

		r2.Push();
		r2.Translate(contentBounds.Position);
		r2.TextShadowColor(GetStyle().ShadowColor);
		r2.Color(GetForegroundColor());
		r2.DrawStringShadow(Assets::GetInstance().Get<Font>(AssetName::FontMain), Text);
		r2.Pop();
	}

}