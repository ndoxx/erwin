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
    KLOGGER(create_channel("vulkan", 1));
    KLOGGER(attach_all("ConsoleSink", std::make_unique<kb::klog::ConsoleSink>()));

    EngineCreateInfo info;
    info.application_descriptor.name = window_title_;
    info.application_descriptor.version = {0,1,0};
    info.engine_descriptor.name = "ErwinEngine";
    info.engine_descriptor.version = {0,1,0};
    info.window_props.title = window_title_;
    info.window_props.width = window_width_;
    info.window_props.height = window_height_;
    info.window_props.full_screen = false;
    info.window_props.always_on_top = false;
    info.window_props.vsync = true;
    info.window_props.host = true;
    info.window_props.resizable = true;
    info.window_props.api = api;
    info.renderer_config.max_sample_count = 1;

    std::tie(window_, render_device_, swapchain_, render_context_) = EngineFactory::create(api, info);

    window_->set_window_close_callback([this]() { is_running_ = false; });

    KLOGG("render") << "Renderer is ready." << std::endl;

    return true;
}

void GfxTestApplication::run()
{
    while(is_running_)
    {
        update();
        swapchain_->present();
        window_->poll_events();
    }
}

void GfxTestApplication::shutdown()
{
    // Explicitly destroy engine components
    delete render_context_.release();
    delete swapchain_.release();
    delete render_device_.release();
    delete window_.release();
}

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