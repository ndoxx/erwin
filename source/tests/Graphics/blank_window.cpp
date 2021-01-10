#include "common/test_application.h"

class BlankWindowApp : public GfxTestApplication
{
public:
    BlankWindowApp() : GfxTestApplication("Blank window test", 640, 480) {}
    ~BlankWindowApp() {}

protected:
    void update() override { /* nop */ }
};

GfxTestApplication* create_application() { return new BlankWindowApp(); }
