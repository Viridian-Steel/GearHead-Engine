#pragma once
#include "Core.h"
#include "Events/Event.h"
#include "Window.h" 

namespace GearHead {

    class GEARHEAD_API Application {
    public:
        Application();
        virtual ~Application();

        void Run();

    private:
        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
    };
    
    Application* CreateApplication(); //defined in CLIENT
} // namespace GearHead
