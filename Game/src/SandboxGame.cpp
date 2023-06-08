#include <GearHead.hpp>

class SandBox : public GearHead::Application{
public:
    SandBox(){}
    ~SandBox(){}
};

GearHead::Application* GearHead::CreateApplication() {
	GEARHEAD_INFO("This is from the Game Side Lol");
    return new SandBox();
}
