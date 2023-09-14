#pragma once


#include "ghpch.hpp"

#include "GearHead/Core.hpp"
#include "GearHead/Events/Event.hpp"


// Interface For desktop system Window


namespace GearHead {

	struct WindowProps {
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::string& title = "GearHead Engine", unsigned int width = 1280, unsigned int height = 720) 
		: Title(title), Width(width), Height(height) {}
	};

	class GEARHEAD_API Window {
	public:


		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {} // will use this as cleanup

		virtual int ShouldClose() = 0;
		
		virtual void OnUpdate() = 0;

		virtual void DrawFrame() = 0;


		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;


		//Window Attributes

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool eneabled) = 0;
		virtual bool IsVSync() const = 0;


		static Window* Create(const WindowProps& props = WindowProps());

	protected:
		bool isInitialized{false};

	};
}
