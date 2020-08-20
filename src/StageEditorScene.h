#pragma once

#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

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

	enum class UIPanelLayout
	{
		Horizontal, Vertical, Fill, Free
	};

	namespace UIDefaultStyle
	{
		constexpr glm::vec4 ShadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };
		constexpr glm::vec4 IconButtonDefaultTint = { 0.25f, 0.25f, 0.25f, 1.0f };
		constexpr glm::vec4 PanelBackground = { 0.0f, 0.0f, 0.0f, 0.0f };
	}

	enum class StageEditorTool
	{
		BlueSphere,
		RedSphere,
		YellowSphere,
		Bumper,
		Ring
	};


	class UIElement : public EventReceiver
	{
	public:
		UIElement();
		UIElement(UIElement&) = delete;
		UIElement(UIElement&&) = delete;
		virtual ~UIElement() {}

		Rect Bounds;
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 PreferredSize = { 0.0f, 0.0f };
		glm::vec2 MinSize = { 0.0f, 0.0f };
		glm::vec2 MaxSize = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		bool Hovered = false;

		EventEmitter<MouseEvent> MouseDragged, MouseMoved, MouseClicked;
		EventEmitter<WheelEvent> Wheel;

		virtual void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) = 0;
		virtual void Render(Renderer2D& renderer, const Time& time) = 0;
		uint32_t GetId() const { return m_Id; }

		virtual const std::vector<Ref<UIElement>>& Children();

		void Traverse(const std::function<void(UIElement&)>& action);
		
	private:
		static uint32_t m_NextId;
		uint32_t m_Id;
	};

	class UIRoot : public EventReceiver
	{
	public:

		void Attach(Application& app);
		void Detach(Application& app);

		void Render(const glm::vec2& windowSize, const glm::vec2& viewport, Renderer2D& renderer, const Time& time);
		void PushLayer(const Ref<UIPanel>& layer) { m_Layers.push_back(layer); }
		void PopLayer() { m_Layers.pop_back(); }
	private:
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

		std::unordered_map<MouseButton, MouseButtonState> m_MouseButtonState;

		glm::vec2 GetMousePosition(const glm::vec2& windowMousePos) const;
		Ref<UIElement> Intersect(const Ref<UIElement>& root, const glm::vec2& pos, const std::function<bool(const UIElement&)>& predicate = nullptr);

	};

	class UIIconButton : public UIElement
	{
	public:
		UIIconButton() = default;
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
		glm::vec4 BackgroundColor = UIDefaultStyle::PanelBackground;

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

		void Render(Renderer2D& renderer, const Time& time) override;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;

		void SetStage(const Ref<Stage>& stage) { m_Stage = stage; UpdatePattern(); }
		void UpdatePattern();

	private:

		// StageObject: { Texture, Tint }
		std::unordered_map<EStageObject, std::tuple<Ref<Texture2D>, glm::vec4>> m_StageObjRendering;

		std::optional<glm::ivec2> GetStageCoordinates(const glm::vec2 pos) const;

		float m_MinZoom = 0.0f, m_MaxZoom = std::numeric_limits<float>::max();

		glm::vec2 m_ViewOrigin = { 0.0f, 0.0f };
		float m_Zoom = 1.0f;

		Ref<Stage> m_Stage = nullptr;
		Ref<Texture2D> m_Pattern = nullptr;
	};

	class UITextInput : public UIElement
	{
	public:
		
		EventEmitter<std::string> ValueChanged;
		void UpdateBounds(const glm::vec2& origin, const glm::vec2& computedSize) override;
		void Render(Renderer2D& renderer, const Time& time) override;
		void SetValue(const std::string& value) { m_Value = value; }
		const std::string& GetValue() const { return m_Value; }

	private:
		std::string m_Value;
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