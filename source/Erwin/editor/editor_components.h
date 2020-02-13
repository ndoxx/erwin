#pragma once

#include "erwin.h"
#include "glm/glm.hpp"

namespace erwin
{

class ComponentEditorSelection: public Component
{
public:
	COMPONENT_DECLARATION(ComponentEditorSelection);

	ComponentEditorSelection();

	virtual bool init(void* description) override final { return true; }
	virtual void inspector_GUI() override final { }

};

} // namespace erwin