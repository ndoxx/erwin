#pragma once

#include "editor/widget.h"

namespace erwin::memory::debug
{
	struct AreaItem;
}

namespace editor
{

class HexDumpWidget: public Widget
{
public:
	HexDumpWidget();
	virtual ~HexDumpWidget();

	void refresh();

protected:
	void register_area_description(const std::string& name, const std::vector<erwin::memory::debug::AreaItem>& items);
	virtual void on_imgui_render() override;

private:

};

} // namespace editor