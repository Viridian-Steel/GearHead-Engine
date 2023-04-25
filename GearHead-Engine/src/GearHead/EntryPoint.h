#pragma once

#ifdef GEARHEAD_PLATFORM_WINDOWS

extern GearHead::Application* GearHead::CreateApplication();

int main( int argc, char** argv) {

    GearHead::Log::Init();
    GEARHEAD_CORE_WARN("Initialized Log");
    int a = 5;
    GEARHEAD_INFO("TESTING! Var={0}",a);

    auto app = GearHead::CreateApplication();
    app->Run();
    delete app;
}


#endif