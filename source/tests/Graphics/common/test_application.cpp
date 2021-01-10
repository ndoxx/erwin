#include "test_application.h"
#include <kibble/argparse/argparse.h>
#include <kibble/logger/dispatcher.h>
#include <kibble/logger/logger.h>
#include <kibble/logger/sink.h>

GfxTestApplication::GfxTestApplication(const std::string& window_title, uint32_t width, uint32_t height)
    : window_width_(width), window_height_(height), window_title_(window_title)
{}

bool GfxTestApplication::init(DeviceAPI api)
{
    KLOGGER(create_channel("render", 3));
    KLOGGER(attach_all("ConsoleSink", std::make_unique<kb::klog::ConsoleSink>()));

    WindowProps props;
    props.title = window_title_;
    props.width = window_width_;
    props.height = window_height_;
    props.full_screen = false;
    props.always_on_top = false;
    props.vsync = true;
    props.host = true;

    window_ = Window::create(api, props);
    window_->set_window_close_callback([this]() { is_running_ = false; });

    render_device_ = RenderDevice::create(api, *window_);
    swap_chain_ = SwapChain::create(api, *window_, *render_device_);

    return true;
}

void GfxTestApplication::run()
{
    while(is_running_)
    {
        update();
        swap_chain_->present();
        window_->poll_events();
    }
}

void GfxTestApplication::shutdown() {}

// ENTRY POINT
extern GfxTestApplication* create_application();

using namespace kb;

void show_error_and_die(ap::ArgParse& parser)
{
    for(const auto& msg : parser.get_errors())
        KLOGW("core") << msg << std::endl;

    KLOG("core", 1) << parser.usage() << std::endl;
    exit(0);
}

int main(int argc, char** argv)
{
    KLOGGER_START();

    ap::ArgParse parser("GfxTest", "0.1");

    const auto& a_api = parser.add_variable<std::string>('a', "api", "Graphics API", "gl");

    bool success = parser.parse(argc, argv);
    if(!success)
        show_error_and_die(parser);

    DeviceAPI api;
    if(!a_api().compare("gl"))
        api = DeviceAPI::OpenGL;
    else if(!a_api().compare("vk"))
        api = DeviceAPI::Vulkan;
    else
    {
        KLOGF("core") << "Unknown API: " << a_api() << std::endl;
        exit(0);
    }

    auto app = create_application();
    if(!app->init(api))
    {
        delete app;
        return -1;
    }

    app->run();
    app->shutdown();
    delete app;

    return 0;
}