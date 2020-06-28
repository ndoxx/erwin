#pragma once

#include "entity/reflection.h"

namespace editor
{
	
struct SelectedTag {};
struct FixedHierarchyTag {};
struct NonEditableTag {};
struct NonRemovableTag {};
struct NoGizmoTag {};
struct GizmoHandleSelectedTag {};
struct GizmoHandleComponent
{
    int handle_id = -1;
    erwin::EntityID parent;
};
struct RayHitTag
{
	float near = 0.f;
};

} // namespace editor