#pragma once

#include "widget.h"
#include "debug/logger_sink.h"
#include <vector>

namespace editor
{

class ConsoleWidget: public Widget
{
public:
	ConsoleWidget();
	virtual ~ConsoleWidget();

	void push(const std::string& message);
	int text_edit_callback(void* data);

protected:
	virtual void on_render() override;

private:
    char input_buffer_[256];
    std::vector<std::string> items_;

    bool auto_scroll_;
    bool scroll_to_bottom_;
};

// This logger sink writes to the console widget
class ConsoleWidgetSink: public erwin::dbg::Sink
{
public:
	ConsoleWidgetSink(ConsoleWidget* p_console);

	virtual ~ConsoleWidgetSink() = default;
	virtual void send(const erwin::dbg::LogStatement& stmt, const erwin::dbg::LogChannel& chan) override;
	virtual void send_raw(const std::string& message) override;

private:
	ConsoleWidget* console_;
};

} // namespace editor