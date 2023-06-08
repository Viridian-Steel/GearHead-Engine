#include "ghpch.hpp"

#include "Application.hpp"
#include "Events/ApplicationEvent.hpp"
#include "Log.hpp"



namespace GearHead{
    Application::Application() {
		        
    }

    Application::~Application(){

    }
    
    void Application::Run(){
		while (ghWindow.ShouldClose()) {
			
			ghWindow.OnUpdate();
		}
		GEARHEAD_CORE_CRITICAL("APP CLOSED");
    }

}
