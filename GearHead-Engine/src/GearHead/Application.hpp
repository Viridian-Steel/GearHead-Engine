#pragma once
#include "Core.hpp"
#include "Render/GHWindow.hpp"
#include "Render/GHDevice.hpp"

#include "Events/Event.hpp"


namespace GearHead {

    class GEARHEAD_API Application {
    public:
		static constexpr int initwidth = 800;
		static constexpr int initheight = 600;

        Application();
        virtual ~Application();

        void Run();



    private:
		GHWindow ghWindow{ initwidth, initheight, true, "U work?"};
		GHDevice ghDevice{ ghWindow };
		
        bool m_Running = true;
    };
    
    Application* CreateApplication(); //defined in CLIENT
} // namespace GearHead
