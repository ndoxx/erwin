#pragma once

#include "event/event.h"
#include "render/handles.h"
#include "asset/bounding.h"
#include "glm/glm.hpp"
#include "entity/reflection.h"

namespace erwin
{

struct MeshSwitchEvent: public WEvent
{
	EVENT_DECLARATION(MeshSwitchEvent);
    MeshSwitchEvent() = default;
    
#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "(void)";
    }
#endif

    EntityID entity;
	VertexArrayHandle VAO;
	VertexBufferLayoutHandle layout;
	Extent extent;
};

} // namespace erwin