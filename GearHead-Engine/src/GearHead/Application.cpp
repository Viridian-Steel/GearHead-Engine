#include "ghpch.h"

#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"


namespace GearHead{
    Application::Application() {

    }

    Application::~Application(){

    }
    
    void Application::Run(){

        WindowResizeEvent e(1280, 720);
        if (e.IsInCategory(EventCategoryApplication)) {
            GEARHEAD_TRACE(e);
        }

        if (e.IsInCategory(EventCategoryInput)) {
            GEARHEAD_TRACE(e);
        }

        while (true);
    }

}