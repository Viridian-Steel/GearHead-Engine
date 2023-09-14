#include "ghpch.hpp"

#include "Application.hpp"
#include "Events/ApplicationEvent.hpp"
#include "Log.hpp"



namespace GearHead{
    Application::Application() {
		window = std::unique_ptr<Window>(Window::Create());
    }

    Application::~Application(){

    }
    
    void Application::Run(){
		

		while (window->ShouldClose()) {
			window->OnUpdate();
		}

		//while (ghWindow.ShouldClose()) {
		//	
		//	ghWindow.OnUpdate();
		//}
		GEARHEAD_CORE_CRITICAL("APP CLOSED");
    }

}
