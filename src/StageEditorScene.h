#pragma once

#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <sstream>

#include "Scene.h"
#include "Stage.h"
#include "EventEmitter.h"

namespace bsf
{
	class Application;
	class Renderer2D;
	class Texture2D;

	class UIPanel;
	class UIElement;
	class UIPanel;
	class UIRoot;
	
	enum class StageEditorTool
	{
		BlueSphere,
		RedSphere,
		YellowSphere,
		Bumper,
		Ring
	};

	enum class UIPanelLayout
	{
		Horizontal, Vertical, Fill, Free
	};



	struct UIPalette
	{
		glm::vec4 Foreground = Colors::White;
		glm::vec4 Background = ToColor(0xff121212);

		glm::vec4 Primary = Colors::Blue;
		glm::vec4 PrimaryContrast = Colors::White;

		glm::vec4 Secondary = Colors::Yellow;
		glm::vec4 SecondaryContrast = Colors::Black;

	};

	struct UIStyle
	{

		UIStyle() { Recompute(); }
		// Sizes
		float GlobalScale = 20.0f;
		
		float MarginUnit = 0.0f;
		
		float IconButtonShadowOffset = 0.0f;

		float StageAreaShadowOffset = 0.075f;
		float StageAreaCrosshairSize = 0.3f;
		float StageAreaCrosshairThickness = 0.1f;

		float TextInputShadowOffset = 0.025f;
		float TextInputLabelFontScale = 0.5f;
		float TextInputMargin = 1.0f;

		float TextMargin = 1.0f;

		// Colors
		glm::vec4 ShadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };
		glm::vec4 IconButtonDefaultTint = { 0.5f, 0.5f, 0.5f, 1.0f };

		UIPalette Palette;

		float ComputeMargin(float units) const { return units * MarginUnit; }
		void Recompute();
	};


	enum class UIElementFlags : uint32_t
	{
		None = 0x0,
		ReceiveHover = 0x1,
		ReceiveFocus = 0x2
	};

	class UIElement : public EventReceiver
	{
	public:
		UIElement();
		UIElement(std::underlying_type_t<UIElementFlags> flags);
		UIElement(UIElement&) = delete;
		UIElement(UIElement&&) = delete;
		virtual ~UIElement() = default;

		Rect Bounds;
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 PreferredSize = { 0.0f, 0.0f };
		glm::vec2 MinSize = { 0.0f, 0.0f };
		glm::vec2 MaxSize = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		bool Hovered = false;
		bool Focused = false;
		std::optional<glm::vec4> BackgroundColor = std::nullopt;
		std::optional<glm::vec4> ForegroundColor = std::nullopt;

		EventEmitter<KeyPressedEvent> KeyPressed;
		EventEmitter<KeyReleasedEvent> KeyReleased;
		EventEmitter<MouseEvent> MouseDragged, MouseMoved, MouseClicked;
		EventEmitter<WheelEvent> Wheel;

		virtual void Update(const Time& time) {}
		virtual void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) = 0;
		virtual void Render(Renderer2D& renderer, const Time& time) = 0;

		uint32_t GetId() const { return m_Id; }

		bool GetFlag(UIElementFlags flag) const;

		virtual const std::vector<Ref<UIElement>>& Children();

		void Traverse(const std::function<void(UIElement&)>& action);

		const glm::vec4 GetBackgroundColor() const { return BackgroundColor.has_value() ? BackgroundColor.value() : GetStyle().Palette.Background; }
		const glm::vec4 GetForegroundColor() const { return ForegroundColor.has_value() ? ForegroundColor.value() : GetStyle().Palette.Foreground; }

		const UIStyle& GetStyle() const { return *m_Style; }
		
	private:
		friend class UIRoot;
		static uint32_t m_NextId;
		
		std::underlying_type_t<UIElementFlags> m_Flags;
		UIStyle* m_Style = nullptr;
		uint32_t m_Id;
	};

	class UIRoot : public EventReceiver
	{
	public:

		void SetStyle(const UIStyle& style);

		void Attach(Application& app);
		void Detach(Application& app);

		void Render(const glm::vec2& windowSize, const glm::vec2& viewport, Renderer2D& renderer, const Time& time);
		void PushLayer(const Ref<UIPanel>& layer) { m_Layers.push_back(layer); }
		void PopLayer() { m_Layers.pop_back(); }
	private:

		UIStyle m_Style;

		glm::vec2 m_Viewport, m_WindowSize;
		glm::mat4 m_Projection, m_InverseProjection;
		std::list<Ref<UIPanel>> m_Layers;

		struct MouseButtonState
		{
			MouseButtonState() = default;
			MouseButtonState(MouseButtonState&) = delete;
			MouseButtonState(MouseButtonState&&) = delete;
			bool Pressed = false;
			std::chrono::system_clock::time_point TimePressed;
		};
		
		struct {
			Ref<UIElement> HoverTarget = nullptr;
			Ref<UIElement> DragTarget = nullptr;
			MouseButton DragButton = MouseButton::None;
			glm::vec2 Position = { 0.0f, 0.0f }, PrevPosition = { 0.0f, 0.0f };
		} m_MouseState;

		Ref<UIElement> m_FocusedControl;

		std::unordered_map<MouseButton, MouseButtonState> m_MouseButtonState;

		glm::vec2 GetMousePosition(const glm::vec2& windowMousePos) const;
		Ref<UIElement> Intersect(const Ref<UIElement>& root, const glm::vec2& pos, const std::function<bool(const UIElement&)>& predicate = nullptr);

	};


	class UIIconButton : public UIElement
	{
	public:
		UIIconButton(): UIElement(MakeFlags(UIElementFlags::ReceiveHover)) {}
		Ref<Texture2D> Icon = nullptr;
		glm::vec4 Tint = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool Selected = false;

		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(Renderer2D& renderer, const Time& time) override;
	private:

	};

	class UIPanel : public UIElement
	{
	public:

		UIPanel() = default;
		UIPanelLayout Layout = UIPanelLayout::Vertical;

		struct {
			float Left = 0.0f;
			float Right = 0.0f;
			float Top = 0.0f;
			float Bottom = 0.0f;
		} Margin;

		void SetMargin(float margin);

		void AddChild(const Ref<UIElement>& child);
		void RemoveChild(const Ref<UIElement>& child);

		void Render(Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;

		const std::vector<Ref<UIElement>>& Children() override { return m_Children; }

	private:
		std::vector<Ref<UIElement>> m_Children;
	};

	class UIStageEditorArea : public UIElement
	{
	public:

		StageEditorTool ActiveTool = StageEditorTool::BlueSphere;

		UIStageEditorArea();

		void Update(const Time& time) override;
		void Render(Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;

		void SetStage(const Ref<Stage>& stage) { m_Stage = stage; UpdatePattern(); }
		void UpdatePattern();

	private:

		void DrawCursor(Renderer2D& r2);

		glm::vec2 GetRawStageCoordinates(const glm::vec2 pos) const;
		std::optional<glm::ivec2> GetStageCoordinates(const glm::vec2 pos) const;

		// StageObject: { Texture, Tint }
		std::unordered_map<EStageObject, std::tuple<Ref<Texture2D>, glm::vec4>> m_StageObjRendering;

		float m_MinZoom = 1.0f, m_MaxZoom = std::numeric_limits<float>::max();

		glm::vec2 m_ViewOrigin = { 0.0f, 0.0f };
		float m_Zoom = 1.0f, m_TargetZoom = 1.0f;

		std::optional<glm::ivec2> m_CursorPos;

		Ref<Stage> m_Stage = nullptr;
		Ref<Texture2D> m_Pattern = nullptr, m_BgPattern;
	};
	
	class UITextInput : public UIElement
	{
	public:
		std::string Label;

		using PushFn = std::function<bool(const std::string&)>;
		using PullFn = std::function<std::string(void)>;

		UITextInput();

		void BindPush(const PushFn& push) { m_Push = push; }
		void BindPull(const PullFn& pull) { m_Pull = pull; }

		void Update(const Time& time) override;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(Renderer2D& renderer, const Time& time) override;

		const std::string& GetValue() const { return m_Value; }

	private:

		PushFn m_Push = [&](const std::string& val) { m_Value = val; return true; };
		PullFn m_Pull = [&] { return m_Value; };

		std::string m_Value;
	};

	class UIText : public UIElement
	{
	public:
		std::string Text;

		void Update(const Time& time) override;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(Renderer2D& renderer, const Time& time) override;

	private:

	};


	class StageEditorScene : public Scene
	{
	public:
		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;

	private:
		Ref<Stage> m_CurrentStage;
		Ref<UIRoot> m_uiRoot;
		Ref<UIStageEditorArea> m_uiEditorArea;

		std::vector<Ref<UIIconButton>> m_ToolButtons;

		void InitializeUI();

	};

	

}