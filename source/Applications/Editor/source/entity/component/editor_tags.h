#pragma once

namespace editor
{

struct SelectedTag {};
struct HiddenTag {};
struct FixedHierarchyTag {};
struct NonEditableTag {};
struct NoGizmoTag {};
struct GizmoUpdateTag {};
struct GizmoDirtyTag {};
struct RayHitTag
{
	float near = 0.f;
};

} // namespace editor