#pragma once

#include "../Erwin/core/window.h"
#include <memory>

namespace erwin
{

class GLFWWindow : public Window
{
public:
    explicit GLFWWindow(const WindowProps& props, EventBus& event_bus);
    ~GLFWWindow();

    virtual void update() override;
    virtual uint32_t get_width() const override;
    virtual uint32_t get_height() const override;

    virtual void set_vsync(bool value) override;
    virtual bool is_vsync() override;

    virtual void* get_native() const override;

private:
    void init(const WindowProps& props);
    void set_event_callbacks(const WindowProps& props);
    void cleanup();

private:
    struct GLFWWindowDataImpl;
    std::unique_ptr<GLFWWindowDataImpl> data_;
};

} // namespace erwin
