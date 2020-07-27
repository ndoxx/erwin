#pragma once

namespace editor
{

struct SelectedTag {};
struct HiddenTag {};
struct FixedHierarchyTag {};
struct NonEditableTag {};
struct NoGizmoTag {};
struct RayHitTag
{
	float near = 0.f;
};

} // namespace editor