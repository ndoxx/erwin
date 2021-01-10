#include "common/test_application.h"
#include <kibble/logger/dispatcher.h>
#include <kibble/logger/sink.h>

class BlankWindowApp : public GfxTestApplication
{
public:
    ~BlankWindowApp() = default;

    bool init() override
    {
        KLOGGER(create_channel("render", 3));
        KLOGGER(attach_all("ConsoleSink", std::make_unique<kb::klog::ConsoleSink>()));

        WindowProps props;
        props.title = "Blank window test";
        props.width = 640;
        props.height = 480;
        props.full_screen = false;
        props.always_on_top = false;
        props.vsync = true;
        props.host = true;

        window_ = Window::create(props);
        window_->set_window_close_callback([this]() { is_running_ = false; });

        render_device_ = RenderDevice::create(DeviceAPI::OpenGL, *window_);

        return true;
    }
    void run() override
    {
        while(is_running_)
        {
            window_->swap_buffers();
            window_->poll_events();
        }
    }
    void shutdown() override {}

private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> render_device_;
    bool is_running_ = true;
};

GfxTestApplication* create_application() { return new BlankWindowApp(); }
