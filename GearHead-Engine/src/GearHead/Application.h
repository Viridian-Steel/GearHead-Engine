#pragma once
#include "Core.h"
#include "Events/Event.h"


namespace GearHead {

    class GEARHEAD_API Application {
        public:
        Application();
        virtual ~Application();

        void Run();
    };
    
    Application* CreateApplication(); //defined in CLIENT
} // namespace GearHead
