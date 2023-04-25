#include <GearHead.h>

class SandBox : public GearHead::Application{
public:
    SandBox(){}
    ~SandBox(){}
};

GearHead::Application* GearHead::CreateApplication() {
    return new SandBox();
}