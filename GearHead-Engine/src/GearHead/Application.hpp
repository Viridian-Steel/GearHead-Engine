#pragma once
#include "Core.hpp"


#include "Events/Event.hpp"
#include "Window.hpp"


namespace GearHead {

    class GEARHEAD_API Application {
    public:
		static constexpr int initwidth = 800;
		static constexpr int initheight = 600;

        Application();
        virtual ~Application();

        void Run();



    private:
		
		std::unique_ptr<Window> window;

        bool m_Running = true;
    };
    
    Application* CreateApplication(); //defined in CLIENT
} // namespace GearHead
